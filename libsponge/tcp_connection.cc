#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return {}; }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _crt_time - _newest_seg_time; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    if (seg.header().rst) {
        // special case, reset both of inbound and outbound stream
        _reset(false);
        return;
    }
    check_rcv_fin(seg);
    _receiver.segment_received(seg);
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }
    if (_sender.next_seqno_absolute() > 0) {
        if (seg.length_in_sequence_space() > 0) {
            _sender.send_empty_segment();
        } else if (_receiver.ackno().has_value() && seg.header().seqno == _receiver.ackno().value() - 1) {
            _sender.send_empty_segment();
        }
    } else if (seg.header().syn) {
        _sender.fill_window();
    }
    // sync the _newest_seg_time
    _newest_seg_time = _crt_time;
    send_segs();
}

bool TCPConnection::active() const {
    // stream.input_ended to implement
    bool res = (!(_self_fin_acked && _peer_fin_acking && (!_lingering))) && (!_rst);
    return res;
}

void TCPConnection::send_segs() {
    queue<TCPSegment> &seg_queue = _sender.segments_out();
    while (!seg_queue.empty()) {
        auto seg = seg_queue.front();
        fill_seg_fields(seg);
        check_send_fin(seg);
        seg_queue.pop();
        _segments_out.push(seg);
    }
}

void TCPConnection::fill_seg_fields(TCPSegment &seg) {
    TCPHeader &header = seg.header();
    header.win = _receiver.window_size();
    auto ackno = _receiver.ackno();
    if (ackno.has_value()) {
        header.ack = true;
        header.ackno = ackno.value();
    }
}

void TCPConnection::check_send_fin(const TCPSegment &seg) {
    if (seg.header().fin) {
        if (!_first_receive_fin) {
            // active close
            _first_send_fin = true;
        }
        _self_fin_seqno = unwrap_in_connection(seg.header().seqno) + seg.length_in_sequence_space();
    }
    if (_peer_fin_seqno > 0 && seg.header().ack) {
        if (unwrap_in_connection(seg.header().ackno) >= _peer_fin_seqno) {
            _peer_fin_acking = true;
            _check_linger();
        }
    }
}

void TCPConnection::check_rcv_fin(const TCPSegment &seg) {
    if (seg.header().fin) {
        if (!_first_send_fin) {
            // passive close
            _first_receive_fin = true;
            _linger_after_streams_finish = false;
        }
        _peer_fin_seqno = unwrap_in_connection(seg.header().seqno) + seg.length_in_sequence_space();
    }
    if (_self_fin_seqno > 0 && seg.header().ack) {
        if (unwrap_in_connection(seg.header().ackno) >= _self_fin_seqno) {
            _self_fin_acked = true;
            _check_linger();
        }
    }
}

uint64_t TCPConnection::unwrap_in_connection(WrappingInt32 number) {
    return unwrap(number, _cfg.fixed_isn.value(), _sender.next_seqno_absolute());
}

size_t TCPConnection::write(const string &data) { return _sender.stream_in().write(data); }

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _crt_time += ms_since_last_tick;
    if (_sender.fail_count() >= _cfg.MAX_RETX_ATTEMPTS) {
        _reset(true);
        return;
    }
    if (!_lingering) {
        // not lingering, show resend all outstanding
        // segments as usual
        if (_sender.stream_in().bytes_written() == 0 && !_receiver.ackno().has_value())
            return;
        _sender.tick(ms_since_last_tick);
    } else {
        // lingering
        if (_crt_time - _newest_seg_time >= 10 * _cfg.rt_timeout) {
            // end the inbound stream
            _receiver.stream_out().end_input();
            _lingering = false;
        }
    }
    send_segs();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_segs();
}

void TCPConnection::_reset(bool is_active) {
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _linger_after_streams_finish = false;
    _rst = true;
    if (is_active) {
        // should send a RST segment to peer
        TCPSegment seg;
        seg.header().rst = true;
        _segments_out.push(seg);
    }
}

void TCPConnection::_check_linger() {
    if (_self_fin_acked && _peer_fin_acking) {
        if (_linger_after_streams_finish) {
            _lingering = true;
        } else {
            _sender.stream_in().end_input();
            _receiver.stream_out().end_input();
        }
    }
}

void TCPConnection::connect() {
    _do_connect = true;
    _sender.fill_window();
    send_segs();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            // Your code here: need to send a RST segment to the peer
            TCPSegment seg;
            seg.header().rst = true;
            _segments_out.push(seg);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}