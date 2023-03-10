#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    auto header = seg.header();
    if (header.rst) {
        _reassembler.stream_out().error();
    }
    auto seg_length_in_seq_space = seg.length_in_sequence_space();
    if (!_received_syn && header.syn) {
        // in listen state
        // 1. set the isn
        _isn = header.seqno;
        _ack_no_64 = unwrap(header.seqno, _isn, 0ll) + 1;  // update the _ack_no
        // 2. change state of receiver
        _state = 1;
        _received_syn = true;
    }
    if (_state == 1 || _state == 2) {
        auto data = seg.payload().copy();
        // in syn_receive state
        // 1. parse the abs_seqno
        auto abs_seq = unwrap(header.seqno, _isn, _ack_no_64);
        if (data.length() > 0) {
            // cut off the data
            auto right_bound = _ack_no_64 + window_size();
            if (abs_seq + seg_length_in_seq_space > right_bound) {
                auto cut_length = abs_seq + seg_length_in_seq_space - right_bound;
                data = data.substr(0, seg_length_in_seq_space - cut_length);
            }
            auto pre_bytes_written = _reassembler.stream_out().bytes_written();
            // data with the syn segment
            if (!header.syn) {
                _reassembler.push_substring(data, abs_seq - 1, header.fin);
            } else {
                _reassembler.push_substring(data, 0, header.fin);
            }
            auto after_bytes_written = _reassembler.stream_out().bytes_written();
            _ack_no_64 += after_bytes_written - pre_bytes_written;
        }
    }
    if (header.fin) {
        // in fin_receive state
        _state = 2;
        _last_no = unwrap(header.seqno, _isn, _ack_no_64) + seg_length_in_seq_space;
        _received_fin = true;
    }
    if (_received_fin && _ack_no_64 + 1 == _last_no) {
        _ack_no_64 = _last_no;
    }
    if (_ack_no_64 == _last_no && _received_fin) {
        _reassembler.stream_out().end_input();
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_received_syn) {
        return wrap(_ack_no_64, _isn);
    }
    return std::nullopt;
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().bytes_not_read(); }
