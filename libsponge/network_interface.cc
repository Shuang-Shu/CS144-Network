#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    if (_ip_map.count(next_hop_ip)) {
        auto eaddr = _ip_map[next_hop_ip].first;
        send_payload(eaddr, dgram.serialize(), EthernetHeader::TYPE_IPv4);
    } else {
        _unknown_datagram_map[next_hop_ip] = dgram;
        // cerr << "MY DEBUG: broadcast for" << dgram.header().summary()
        //      << ", _unknown_datagram_map count=" << _unknown_datagram_map.count(next_hop_ip)
        //      << "with ip=" << next_hop.ip() << endl;
        broad_cast(next_hop);
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    if (frame.header().dst != _ethernet_address && frame.header().dst != ETHERNET_BROADCAST) {
        return {};
    }
    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram dgarm;
        if (ParseResult::NoError == dgarm.parse(frame.payload())) {
            // if (dgarm.header().dst != _ip_address.ipv4_numeric()) {
            //     return {};
            // }
            return dgarm;
        }
    } else if (frame.header().type == EthernetHeader::TYPE_ARP) {
        ARPMessage msg;
        if (ParseResult::NoError == msg.parse(frame.payload())) {
            // log the ethernet addr and ip addr of source
            auto s_ip = msg.sender_ip_address;
            auto s_eaddr = msg.sender_ethernet_address;
            _ip_map[s_ip] = pair<EthernetAddress, uint64_t>(s_eaddr, _crt_time + MAPPING_EXPIRE_TIME_MS);
            // cerr << "MY DEBUG: receive ARP reply, dgram count=" << _unknown_datagram_map.count(s_ip) << endl;
            if (_unknown_datagram_map.count(s_ip)) {
                // cerr << "MY DEBUG: send new" << endl;
                send_payload(s_eaddr, _unknown_datagram_map[s_ip].serialize(), EthernetHeader::TYPE_IPv4);
                _unknown_datagram_map.erase(s_ip);
            }
            auto t_ip = msg.target_ip_address;
            if (msg.opcode == 2) {
                // if the arp is a reply
                // send waiting dgram if necessary
                auto t_eaddr = msg.target_ethernet_address;
                // cerr << "MY DEBUG: receive ARP reply, dgram count=" << _unknown_datagram_map.count(t_ip) << endl;
                _ip_map[t_ip] = pair<EthernetAddress, uint64_t>(t_eaddr, _crt_time + MAPPING_EXPIRE_TIME_MS);
                if (_unknown_datagram_map.count(t_ip)) {
                    send_payload(t_eaddr, _unknown_datagram_map[t_ip].serialize(), EthernetHeader::TYPE_IPv4);
                    _unknown_datagram_map.erase(t_ip);
                }
            } else if (msg.opcode == 1) {
                if (t_ip == _ip_address.ipv4_numeric()) {
                    ARPMessage reply_msg;
                    reply_msg.opcode = 2;  // arp type is reply
                    reply_msg.sender_ethernet_address = _ethernet_address;
                    reply_msg.sender_ip_address = _ip_address.ipv4_numeric();
                    reply_msg.target_ethernet_address = s_eaddr;
                    reply_msg.target_ip_address = s_ip;
                    send_payload(s_eaddr, reply_msg.serialize(), EthernetHeader::TYPE_ARP);
                }
            }
        } else {
            // cerr << "MY DEBUG: parse ArpMessage error\n";
        }
    }
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    _crt_time += ms_since_last_tick;
    flush_expire_mapping();
    flush_expire_arp_log();
}

void NetworkInterface::broad_cast(Address addr) {
    if (_ip_arp_time.count(addr.ipv4_numeric())) {
        return;
    }
    ARPMessage msg;
    msg.opcode = 1;  // arp type is request
    msg.sender_ethernet_address = _ethernet_address;
    msg.sender_ip_address = _ip_address.ipv4_numeric();
    msg.target_ip_address = addr.ipv4_numeric();
    send_payload(ETHERNET_BROADCAST, msg.serialize(), EthernetHeader::TYPE_ARP);
    // cerr << "MY DEBUG: crt_time=" << _crt_time << ", expire_time=" << _crt_time + DST_IP_EXPIRE_TIME_MS << endl;
    _ip_arp_time[addr.ipv4_numeric()] = _crt_time + DST_IP_EXPIRE_TIME_MS;
}

void NetworkInterface::flush_expire_mapping() {
    for (auto it = _ip_map.begin(); it != _ip_map.end();) {
        pair<EthernetAddress, uint64_t> ele = it->second;
        if (ele.second <= _crt_time) {
            it = _ip_map.erase(it);
        } else {
            it++;
        }
    }
}

void NetworkInterface::flush_expire_arp_log() {
    for (auto it = _ip_arp_time.begin(); it != _ip_arp_time.end();) {
        // cerr << "MY DEBUG: crt_time=" << _crt_time << ", ip=" << Address::from_ipv4_numeric(it->first).ip()
        //      << ", expire_time=" << it->second << endl;
        if (it->second <= _crt_time) {
            it = _ip_arp_time.erase(it);
        } else {
            it++;
        }
    }
}

void NetworkInterface::send_payload(const EthernetAddress &eaddr, BufferList payload, uint16_t type) {
    EthernetFrame eframe;
    // set the header
    eframe.header().dst = eaddr;
    eframe.header().type = type;
    eframe.header().src = _ethernet_address;
    // set the payload
    eframe.payload() = payload;
    _frames_out.push(eframe);
}
