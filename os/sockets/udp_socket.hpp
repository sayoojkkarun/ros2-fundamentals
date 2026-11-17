#pragma once

#include <netinet/in.h>   // sockaddr_in, ip_mreq
#include <sys/socket.h>   // setsockopt

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

[[noreturn]] void throw_errno(const char * what);

class UdpSocket
{
public:
  UdpSocket();
  ~UdpSocket();
  UdpSocket(const UdpSocket &) = delete;
  UdpSocket & operator=(const UdpSocket &) = delete;
  UdpSocket(UdpSocket && other) noexcept;
  UdpSocket & operator=(UdpSocket && other) noexcept;

  template<typename T>
  void set_option(int level, int name, const T & value)
  {
    if (::setsockopt(fd_, level, name, &value, sizeof(value)) == -1) {
      throw_errno("setsockopt");
    }
  }

  void bind_any(std::uint16_t port);
  void join_group(const std::string & group);
  void send_to(const std::string & group, std::uint16_t port, const std::string & message);
  [[nodiscard]] std::size_t receive_from(std::vector<unsigned char> & buf, sockaddr_in & from);
  // Like receive_from, but returns std::nullopt instead of throwing when SO_RCVTIMEO
  // expires (EAGAIN/EWOULDBLOCK) or a signal interrupts the wait (EINTR).
  [[nodiscard]] std::optional<std::size_t> try_receive_from(
    std::vector<unsigned char> & buf, sockaddr_in & from);

private:
  /* Socket file descriptor */
  int fd_ = -1;
};
