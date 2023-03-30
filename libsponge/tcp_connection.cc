#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const {
    // remaining_outbound_capacity
    return _sender.peer_win();
}

size_t TCPConnection::bytes_in_flight() const {
    // bytes_in_flight
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const {
    // unassembled_bytes
    return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const {
    // time_since_last_segment_received
    return _crt_time - _last_seg_rcv_time;
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    if (seg.header().rst) {
        // special case, reset both of inbound and outbound stream
        _reset();
        return;
    }
    _receiver.segment_received(seg);
    if (_fin && _receiver.ackno().value() == _sender.next_seqno() && !_linger_after_streams_finish) {
        // passive close connection
        _receiver.stream_out().end_input();
    }
    // notify the sender
    auto header = seg.header();
    // ACK_RECEIVED could need send a new seg, because _sender may did it
    _sender.ack_received(_receiver.ackno().value(), header.win);
    _send_segs();
    if (_receiver.ackno().has_value() && (seg.length_in_sequence_space() == 0) &&
        (seg.header().seqno == _receiver.ackno().value() - 1)) {
        _sender.send_empty_segment();
    }
    // send at least one segment when incoming segment
    // occupied any sequence numbers
    if (seg.length_in_sequence_space() > 0) {
        TCPSegment empty_seg;
        // seg ackno and window_size
        empty_seg.header().ack = true;
        empty_seg.header().ackno = _receiver.ackno().value();
        empty_seg.header().win = _receiver.window_size();
        _segments_out.push(empty_seg);
    }
    // sync the _last_seg_rcv_time
    _last_seg_rcv_time = _crt_time;
}

bool TCPConnection::active() const {
    // active()
    auto res = (!_receiver.stream_out().input_ended() && !_sender.stream_in().input_ended()) | _is_lingering;
    cout << "_rcv: " << _receiver.stream_out().input_ended() << ", _sd: " << _sender.stream_in().input_ended() << endl;
    return res;
}

size_t TCPConnection::write(const string &data) {
    // write()
    return _sender.stream_in().write(data);
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _crt_time += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    _send_segs();
    // TICK could need send a new seg, because _sender may did it
    if (_sender.fail_count() > _cfg.MAX_RETX_ATTEMPTS) {
        _reset();
        return;
    }

    if (_crt_time - _last_seg_rcv_time >= 10 * _cfg.rt_timeout) {
        _receiver.stream_out().end_input();
        _is_lingering = false;
    }
}

void TCPConnection::end_input_stream() {
    // Shut down the outbound byte stream
    _sender.stream_in().end_input();
    // notify the _sender
    _sender.fill_window();
    _send_segs();
}

void TCPConnection::connect() {
    // connect()
    _sender.fill_window();
    _send_segs();
}

void TCPConnection::_send_segs() {
    // send all segments
    queue<TCPSegment> &seg_queue = _sender.segments_out();
    while (!seg_queue.empty()) {
        // check FIN flag of segments
        auto seg = seg_queue.front();
        cout << "before size: " << seg_queue.size() << endl;
        seg_queue.pop();
        cout << "after size: " << seg_queue.size() << endl;
        // seg ackno and window_size
        if (_receiver.ackno().has_value()) {
            seg.header().ackno = _receiver.ackno().value();
            seg.header().ack = true;
        }
        seg.header().win = _receiver.window_size();
        // check FIN flag
        if (seg.header().fin && !_fin) {
            _fin = true;
            _sender.stream_in().end_input();
            // log the seqno of fin byte
            _fin_next_seqno = unwrap(seg.header().seqno, _cfg.fixed_isn.value(), _sender.next_seqno_absolute());
            if (_receiver.stream_out().eof()) {
                _linger_after_streams_finish = false;
            }
        }
        if (_fin && (seg.header().seqno + seg.length_in_sequence_space() == _sender.next_seqno()) &&
            _sender.fin_acked(_fin_next_seqno)) {
            _is_lingering = true;
        }
        _segments_out.push(seg);
    }
}

void TCPConnection::_reset() {
    _sender.stream_in().end_input();
    _receiver.stream_out().end_input();
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
