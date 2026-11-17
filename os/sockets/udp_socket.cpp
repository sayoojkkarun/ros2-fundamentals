#include "udp_socket.hpp"

#include <arpa/inet.h>   // inet_pton, htons, htonl
#include <unistd.h>      // close

#include <cerrno>
#include <stdexcept>     // std::invalid_argument
#include <system_error>  // std::system_error
#include <utility>       // std::exchange

namespace
{

	sockaddr_in make_address(const std::string & ip, std::uint16_t port)
	{
		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
			throw std::invalid_argument("invalid IPv4 address: " + ip);
		}
		return addr;
	}

}  // namespace

void throw_errno(const char * what)
{
	throw std::system_error(errno, std::generic_category(), what);
}

UdpSocket::UdpSocket()
	: fd_(::socket(AF_INET, SOCK_DGRAM, 0))
{
	if (fd_ == -1) {
		throw_errno("socket");
	}
}

UdpSocket::~UdpSocket()
{
	if (fd_ != -1) {
		::close(fd_);
	}
}

UdpSocket::UdpSocket(UdpSocket && other) noexcept
: fd_(std::exchange(other.fd_, -1))
{
}

UdpSocket & UdpSocket::operator=(UdpSocket && other) noexcept
{
	if (this == &other) {
		return *this;
	}

	if (fd_ != -1) {
		::close(fd_);
	}

	fd_ = std::exchange(other.fd_, -1);

	return *this;
}

void UdpSocket::bind_any(std::uint16_t port)
{
	sockaddr_in addr{};

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (::bind(fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
		throw_errno("bind");
	}
}

void UdpSocket::join_group(const std::string & group)
{
	ip_mreq mreq{};

	mreq.imr_multiaddr = make_address(group, 0).sin_addr;
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);

	set_option(IPPROTO_IP, IP_ADD_MEMBERSHIP, mreq);
}

void UdpSocket::send_to(const std::string & group, std::uint16_t port,
			const std::string & message)
{
	const sockaddr_in dest = make_address(group, port);

	const ssize_t sent = ::sendto(fd_, message.data(), message.size(), 0,
				      reinterpret_cast<const sockaddr *>(&dest), sizeof(dest));

	if (sent == -1) {
		throw_errno("sendto");
	}
}

std::size_t UdpSocket::receive_from(std::vector<unsigned char> & buf, sockaddr_in & from)
{
	socklen_t from_len = sizeof(from);

	const ssize_t received = ::recvfrom(fd_, buf.data(), buf.size(), 0,
					    reinterpret_cast<sockaddr *>(&from), &from_len);

	if (received == -1) {
		throw_errno("recvfrom");
	}

	return static_cast<std::size_t>(received);
}

std::optional<std::size_t> UdpSocket::try_receive_from(std::vector<unsigned char> & buf,
						       sockaddr_in & from)
{
	socklen_t from_len = sizeof(from);

	const ssize_t received = ::recvfrom(fd_, buf.data(), buf.size(), 0,
					    reinterpret_cast<sockaddr *>(&from), &from_len);

	if (received == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
			return std::nullopt;   // timeout or signal: not an error
		}
		throw_errno("recvfrom");
	}

	return static_cast<std::size_t>(received);
}
