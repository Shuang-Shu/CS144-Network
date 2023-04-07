#include "router.hh"

#include <iostream>
#include <utility>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num
         << ", next size=" << _map_vec.size() + 1 << endl;

    DUMMY_CODE(route_prefix, prefix_length, next_hop, interface_num);
    // Your code here.
    uint32_t mask = (0xFFFFFFFF) << (32 - prefix_length);
    // cerr << "MY DEBUG: mask=" << Address::from_ipv4_numeric(mask).ip()
    //      << ", masked route=" << Address::from_ipv4_numeric(route_prefix & mask).ip() << endl;
    auto tmp = MapItem((route_prefix & mask), prefix_length, next_hop, interface_num);
    // cerr << "MY DEBUG: item.route_prefix=" << tmp.route_prefix << endl;
    _map_vec.push_back(tmp);
}

bool matched_prefix_length(uint32_t ip, uint32_t pattern, uint8_t prefix_length) {
    // cerr << "MY DEBUG: in_ip=" << Address::from_ipv4_numeric(ip).ip()
    //      << ", pattern=" << Address::from_ipv4_numeric(pattern).ip()
    //      << ", prefix_len=" << static_cast<int>(prefix_length) << endl;
    uint32_t match = ~(ip ^ pattern);
    uint32_t mask = 1L << 31;
    uint8_t matched_len = 0;
    for (int i = 0; i < 32; i++) {
        if (mask & match) {
            mask >>= 1;
            matched_len++;
        } else {
            break;
        }
    }
    return matched_len >= prefix_length ? true : false;
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    DUMMY_CODE(dgram);
    // Your code here.
    // cerr << "MY DEBUG: init ttl=" << static_cast<int>(dgram.header().ttl) << endl;
    if (dgram.header().ttl == 0 || dgram.header().ttl == 1) {
        return;
    } else {
        dgram.header().ttl--;
    }
    uint8_t max_match_length = 0;
    size_t matched_interface_num = 0;
    optional<Address> next_hop = {};
    uint32_t dest_ip = dgram.header().dst;
    for (uint32_t i = 0; i < _map_vec.size(); i++) {
        auto item = _map_vec[i];
        // cerr << "MY DEBUG: dest_ip=" << Address::from_ipv4_numeric(dest_ip).ip()
        //      << ", item.route_prefix=" << Address::from_ipv4_numeric(item.route_prefix).ip() << endl;
        auto matche_prefix = matched_prefix_length(dest_ip, item.route_prefix, item.prefix_length);
        if (matche_prefix) {
            // cerr << "MY DEBUG: MATCHED dest_ip=" << Address::from_ipv4_numeric(dest_ip).ip()
            //      << ", item.route_prefix=" << Address::from_ipv4_numeric(item.route_prefix).ip()
            //      << ", matched len=" << static_cast<int>(item.prefix_length) << endl;
            if (item.route_prefix >= max_match_length) {
                max_match_length = item.route_prefix;
                matched_interface_num = item.interface_num;
                next_hop = item.next_hop;
            }
        }
    }
    // cerr << "MY DEBUG: matched_interface_num=" << matched_interface_num << ", max_match_length=" << max_match_length
    //      << endl;
    // cerr << "MY DEBUG: init ttl=" << static_cast<int>(dgram.header().ttl) << endl;
    if (next_hop.has_value()) {
        _interfaces[matched_interface_num].send_datagram(dgram, next_hop.value());
    } else {
        _interfaces[matched_interface_num].send_datagram(dgram, Address::from_ipv4_numeric(dgram.header().dst));
    }
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
