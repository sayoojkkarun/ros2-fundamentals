#include "safe_queue.hpp"
#include "udp_socket.hpp"

#include <sys/time.h>  // timeval

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace
{
using Clock = std::chrono::steady_clock;

// Set by Ctrl+C (signal handler) or by main. Read by both threads.
std::atomic<bool> g_stop{false};
static_assert(
  std::atomic<bool>::is_always_lock_free,
  "a signal handler may only touch lock-free atomics");

void on_sigint(int)
{
  g_stop = true;
}

struct Message
{
  std::string text;
  Clock::time_point received_at;
};

void receive_loop(UdpSocket & sock, SafeQueue<Message> & queue, std::size_t & received)
{
  std::vector<unsigned char> buf(65536);
  while (!g_stop) {
    sockaddr_in from{};
    const auto n = sock.try_receive_from(buf, from);

    if (!n) {
      continue;
    }

    queue.push(Message{std::string(buf.begin(), buf.begin() + *n), Clock::now()});
    ++received;
  }
}

void executor_loop(
  SafeQueue<Message> & queue, std::chrono::milliseconds callback_time,
  std::size_t & processed)
{
  while (auto msg = queue.pop()) {
    const auto waited = Clock::now() - msg->received_at;
    std::this_thread::sleep_for(callback_time);
    std::cout << "callback: " << msg->text << " | waited "
              << std::chrono::duration_cast<std::chrono::milliseconds>(waited).count()
              << " ms | queue " << queue.size() << " | dropped " << queue.dropped() << std::endl;
    ++processed;
  }
}
}  // namespace

int main(int argc, char ** argv)
{
  const auto depth = static_cast<std::size_t>(argc > 1 ? std::atoi(argv[1]) : 10);
  const std::chrono::milliseconds callback_time(argc > 2 ? std::atoi(argv[2]) : 0);
  const std::chrono::seconds run_time(argc > 3 ? std::atoi(argv[3]) : 0);

  std::signal(SIGINT, on_sigint);

  try {
    // Provided: the receiver.cpp setup, plus a 100 ms receive timeout so the
    // receive thread wakes up regularly and can see g_stop.
    UdpSocket sock;
    sock.set_option(SOL_SOCKET, SO_REUSEADDR, 1);
    sock.bind_any(5000);
    sock.join_group("239.1.2.3");
    sock.set_option(SOL_SOCKET, SO_RCVTIMEO, timeval{0, 100'000});

    SafeQueue<Message> queue(depth);
    std::size_t received = 0;
    std::size_t processed = 0;

    std::cout << "subscriber: depth " << depth << ", callback " << callback_time.count()
              << " ms (Ctrl+C to stop)" << std::endl;

    std::thread rx(receive_loop, std::ref(sock), std::ref(queue), std::ref(received));
    std:: thread ex(executor_loop, std::ref(queue), callback_time, std::ref(processed));

    // Provided: main waits for Ctrl+C or the time limit.
    const auto start = Clock::now();
    while (!g_stop && (run_time.count() == 0 || Clock::now() - start < run_time)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    g_stop = true;
    rx.join();
    queue.close();
    ex.join();

    std::cout << "received " << received << ", processed " << processed
              << ", dropped " << queue.dropped() << std::endl;

  } catch (const std::exception & e) {

    std::cerr << "error: " << e.what() << '\n';
    return 1;

  }

  return 0;
}
