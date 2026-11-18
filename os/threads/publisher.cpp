#include "udp_socket.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char ** argv)
{
  const int count = argc > 1 ? std::atoi(argv[1]) : 100;
  const int rate_hz = argc > 2 ? std::atoi(argv[2]) : 50;
  const auto period = std::chrono::microseconds(1'000'000 / (rate_hz > 0 ? rate_hz : 1));

  try {
    UdpSocket sock;
    sock.set_option(IPPROTO_IP, IP_MULTICAST_TTL, static_cast<unsigned char>(1));
    sock.set_option(IPPROTO_IP, IP_MULTICAST_LOOP, static_cast<unsigned char>(1));

    auto next = std::chrono::steady_clock::now();
    for (int i = 0; i < count; ++i) {
      sock.send_to("239.1.2.3", 5000, "msg " + std::to_string(i));
      next += period;
      std::this_thread::sleep_until(next);
    }
    std::cout << "published " << count << " messages at " << rate_hz << " Hz" << std::endl;
  } catch (const std::exception & e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
