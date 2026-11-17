#include "udp_socket.hpp"

#include <sys/time.h>   // timeval

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
	constexpr const char *kMagic = "TSPDP";
	constexpr int kVersion = 1;
	constexpr const char *kGroup = "239.1.2.3";
	constexpr std::uint16_t kPort = 5000;

	struct Announcement
	{
		std::uint64_t id;
		std::string name;
		std::uint16_t port;
		std::chrono::milliseconds lease;
	};

	std::string to_wire(const Announcement & a)
	{
		std::ostringstream out;

		out << kMagic << ' ' << kVersion << ' '
		    << std::hex << std::setw(16) << std::setfill('0') << a.id << std::dec << ' '
		    << a.name << ' '
		    << a.port << ' '
		    << a.lease.count();

		return out.str();
	}

	std::optional<Announcement> from_wire(const std::string & packet)
	{
		std::istringstream in(packet);

		std::string magic;
		int version = 0;
		Announcement a{};
		long long port = 0;
		long long lease_ms = 0;

		if (!(in >> magic >> version >> std::hex >> a.id >> std::dec
		    >> a.name >> port >> lease_ms)) {
			return std::nullopt;
		}

		if (magic != kMagic || version != kVersion ||
		    port < 0 || port > 65535 || lease_ms <= 0) {
			return std::nullopt;
		}

		a.port = static_cast<std::uint16_t>(port);
		a.lease = std::chrono::milliseconds(lease_ms);

		return a;
	}

	//Peer table
	using Clock = std::chrono::steady_clock;

	struct Peer
	{
		std::string name;
		Clock::time_point last_seen;
		std::chrono::milliseconds lease;
	};

	using PeerTable = std::unordered_map<std::uint64_t, Peer>;

	void update_peer(PeerTable & peers, const Announcement & a, Clock::time_point now)
	{
		const auto [pos, inserted] = peers.insert_or_assign(a.id, Peer{a.name, now, a.lease});
		if (inserted) {
			std::cout << "DISCOVERED " << a.name << " | ID " << std::hex << a.id << std::dec << '\n';
		}
	}

	void expire_peers(PeerTable & peers, Clock::time_point now)
	{
		for (auto it = peers.begin(); it != peers.end(); ) {
			const Peer & peer = it->second;
			if (now - peer.last_seen > peer.lease) {
				std::cout << "LOST " << peer.name << " | ID " << std::hex << it->first << std::dec << '\n';
				it = peers.erase(it);   // erase returns the next element
			} else {
				++it;
			}
		}
	}
} //namespace

int main(int argc, char ** argv)
{
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " <name>\n";
		return EXIT_FAILURE;
	}

	const std::uint16_t unicast_port{8082};
	std::random_device rd;
	std::mt19937_64 rng(rd());

	auto next_announce = Clock::now();
	int sent = 0;
	std::vector<unsigned char> buf(65536);

	const std::string name = argv[1];
	const Announcement self{rng(), name, unicast_port, std::chrono::milliseconds(3000)};
	/* packet is built here */
	const std::string wire = to_wire(self);
	const auto parsed = from_wire(wire);

	if (!parsed) {
		std::cerr << "round trip failed: " << wire << '\n';
		return EXIT_FAILURE;
	}

	std::cout << "wire:   " << wire << '\n'
		  << "parsed: " << parsed->name << " | ID " << std::hex << parsed->id << std::dec
		  << " | port " << parsed->port << " | lease " << parsed->lease.count() << " ms\n";

	UdpSocket sock;
	// For RX
	sock.set_option(SOL_SOCKET, SO_REUSEADDR, 1);
	sock.bind_any(kPort);
	sock.join_group(kGroup);
	sock.set_option(SOL_SOCKET, SO_RCVTIMEO, timeval{0, 100'000});
	// For TX
	sock.set_option(IPPROTO_IP, IP_MULTICAST_TTL, static_cast<unsigned char>(1));
	sock.set_option(IPPROTO_IP, IP_MULTICAST_LOOP, static_cast<unsigned char>(1));

	PeerTable peers;

	while (true) {
		if (Clock::now() >= next_announce) {
			sock.send_to(kGroup, kPort, wire);
			++sent;
			next_announce = Clock::now() +
					std::chrono::milliseconds(sent < 3 ? 100 : 1000);
		}

		sockaddr_in from{};
		const auto n = sock.try_receive_from(buf, from);
		const auto now = Clock::now();   // after the wait, so last_seen is accurate

		if (n) {
			const auto pkt = from_wire(std::string(buf.begin(), buf.begin() + *n));
			if (pkt && pkt->id != self.id) {
				update_peer(peers, *pkt, now);
			}
		}

		expire_peers(peers, now);
	}
}
