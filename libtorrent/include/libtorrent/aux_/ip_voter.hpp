/*

Copyright (c) 2013-2015, 2017, 2019-2021, Arvid Norberg
Copyright (c) 2016, Steven Siloti
Copyright (c) 2016, 2020, Alden Torres
All rights reserved.

You may use, distribute and modify this code under the terms of the BSD license,
see LICENSE file.
*/

#ifndef TORRENT_IP_VOTER_HPP_INCLUDED
#define TORRENT_IP_VOTER_HPP_INCLUDED

#include <vector>
#include "libtorrent/address.hpp"
#include "libtorrent/aux_/bloom_filter.hpp"
#include "libtorrent/time.hpp" // for time_point
#include "libtorrent/aux_/session_interface.hpp" // for ip_source_t

namespace libtorrent::aux {

	// this is an object that keeps the state for a single external IP
	// based on peoples votes
	struct TORRENT_EXTRA_EXPORT ip_voter
	{
		ip_voter();

		// returns true if a different IP is the top vote now
		// i.e. we changed our idea of what our external IP is
		bool cast_vote(address const& ip, aux::ip_source_t source_type, address const& source);

		address external_address() const { return m_external_address; }

	private:

		bool maybe_rotate();

		struct external_ip_t
		{
			bool add_vote(sha1_hash const& k, aux::ip_source_t type);

			// we want to sort descending
			bool operator<(external_ip_t const& rhs) const
			{
				if (num_votes > rhs.num_votes) return true;
				if (num_votes < rhs.num_votes) return false;
				return static_cast<std::uint8_t>(sources) > static_cast<std::uint8_t>(rhs.sources);
			}

			// this is a bloom filter of the IPs that have
			// reported this address
			aux::bloom_filter<16> voters;
			// this is the actual external address
			address addr;
			// a bitmask of sources the reporters have come from
			aux::ip_source_t sources{};
			// the total number of votes for this IP
			std::uint16_t num_votes = 0;
		};

		// this is a bloom filter of all the IPs that have
		// been the first to report an external address. Each
		// IP only gets to add a new item once.
		aux::bloom_filter<32> m_external_address_voters;

		std::vector<external_ip_t> m_external_addresses;
		address m_external_address;

		// the total number of unique IPs that have voted
		int m_total_votes;

		// this is true from the first time we rotate. Before
		// we rotate for the first time, we keep updating the
		// external address as we go, since we don't have any
		// stable setting to fall back on. Once this is true,
		// we stop updating it on the fly, and just use the
		// address from when we rotated.
		bool m_valid_external;

		// the last time we rotated this ip_voter. i.e. threw
		// away all the votes and started from scratch, in case
		// our IP has changed
		time_point m_last_rotate;
	};

	// stores one address for each combination of local/global and ipv4/ipv6
	// use of this class should be avoided, get the IP from the appropriate
	// listen interface wherever possible
	struct TORRENT_EXTRA_EXPORT external_ip
	{
		external_ip()
			: m_addresses{{address_v4(), address_v6()}, {address_v4(), address_v6()}}
		{}

		external_ip(address const& local4, address const& global4
			, address const& local6, address const& global6);

		// the external IP as it would be observed from `ip`
		address external_address(address const& ip) const;

	private:

		// support one local and one global address per address family
		// [0][n] = global [1][n] = local
		// [n][0] = IPv4 [n][1] = IPv6
		// TODO: 1 have one instance per possible subnet, 192.168.x.x, 10.x.x.x, etc.
		address m_addresses[2][2];
	};

}

#endif
