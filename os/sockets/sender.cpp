#include "udp_socket.hpp"

#include <unistd.h>  // getpid

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char ** argv)
{
  const std::string group = argc > 1 ? argv[1] : "239.1.2.3";
  const auto port = static_cast<std::uint16_t>(argc > 2 ? std::atoi(argv[2]) : 5000);

  try {
    UdpSocket sock;

    sock.set_option(IPPROTO_IP, IP_MULTICAST_TTL, static_cast<unsigned char>(1));
    sock.set_option(IPPROTO_IP, IP_MULTICAST_LOOP, static_cast<unsigned char>(1));

    for (int n = 0; n < 10; ++n) {
      const std::string message =
	      "HELO " + std::to_string(n) + " from pid " + std::to_string(::getpid());

      sock.send_to(group, port, message);
      std::cout << "sent to " << group << ':' << port << "  " << message << std::endl;

      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  } catch (const std::exception & e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
