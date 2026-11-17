#include "udp_socket.hpp"

#include <arpa/inet.h>  // inet_ntop, ntohs

#include <array>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

static void dump(const std::vector<unsigned char> & buf, std::size_t n)
{
  const std::size_t lim = n < 16 ? n : 16;
  for (std::size_t i = 0; i < lim; ++i) {
    std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buf[i]) << ' ';
  }
  std::cout << std::dec << " |";
  for (std::size_t i = 0; i < lim; ++i) {
    std::cout << (std::isprint(buf[i]) ? static_cast<char>(buf[i]) : '.');
  }
  std::cout << '|' << std::endl;
}

int main(int argc, char ** argv)
{
  const std::string group = argc > 1 ? argv[1] : "239.1.2.3";
  const auto port = static_cast<std::uint16_t>(argc > 2 ? std::atoi(argv[2]) : 5000);
  std::vector<unsigned char> buf(65536);  // max UDP datagram size

  try {
    UdpSocket sock;

    sock.set_option(SOL_SOCKET, SO_REUSEADDR, 1);
    sock.bind_any(port);
    sock.join_group(group);
    std::cout << "listening on " << group << ':' << port << " (Ctrl+C to stop)" << std::endl;

    while (true) {
      sockaddr_in from{};
      const std::size_t n = sock.receive_from(buf, from);

      std::array<char, INET_ADDRSTRLEN> ip{};
      ::inet_ntop(AF_INET, &from.sin_addr, ip.data(), ip.size());

      std::cout << n << " bytes from " << ip.data() << ':' << ntohs(from.sin_port) << "  ";
      dump(buf, n);
    }
  } catch (const std::exception & e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
