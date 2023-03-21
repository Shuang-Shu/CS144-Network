#include "tcp_sender.hh"

#include "node.hh"
#include "tcp_config.hh"

#include <memory>
#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _outstanding_buffer{} {}

uint64_t TCPSender::bytes_in_flight() const { return {}; }

// get the data
string get_data(ByteStream &byteStream, uint64_t limit) {
    auto available_str = byteStream.peek_output(limit);
    auto available_length = min(available_str.size(), limit);
    auto real_data = available_str.substr(0, available_length);
    byteStream.pop_output(available_length);
    return real_data;
}

void TCPSender::fill_window() {
    string data;
    while ((data = get_data(stream_in(), TCPConfig::MAX_PAYLOAD_SIZE)).length() != 0) {
        TCPSegment seg;
        TCPHeader &header = seg.header();
        if (!_syn_sent) {
            // set SYN flag
            header.syn = true;
            _syn_sent = true;
        }
        if (stream_in().eof()) {
            header.fin = true;
        }
        auto buf = Buffer(move(data));
        _segments_out.push(seg);
        _outstanding_buffer.addTail(seg);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // 1 refresh the window size
    _peer_window_size = window_size;
    // 2 refresh the _acked_next
    auto abs_ackno = unwrap(ackno, _isn, _acked_next);
    // 3 remove outstanding segments in the buffer
    while (true) {
        if (_outstanding_buffer.head == nullptr) {
            break;
        }
        abs_ackno = unwrap(ackno, _isn, _acked_next);
        TCPSegment head_seg = _outstanding_buffer.head->val;
        auto length_in_seq = head_seg.length_in_sequence_space();
        auto head_seg_seqno = unwrap(head_seg.header().seqno, _isn, _acked_next);
        if (abs_ackno > head_seg_seqno + length_in_seq) {
            _outstanding_buffer.pop();
        } else {
            break;
        }
    }
    // reset the timer
    if (_outstanding_buffer.head == nullptr) {
        _timer.stop();
    } else {
        _timer.reset();
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    fill_window();
    _timer.pass(ms_since_last_tick);
    if (_timer.is_expire()) {
        // TODO resent all outstanding segments
        shared_ptr<Node<TCPSegment>> p = _outstanding_buffer.head;
        auto peer_window_size = _peer_window_size;
        while (p->next != nullptr && peer_window_size >= p->next->val.length_in_sequence_space()) {
            p = p->next;
            _segments_out.push(p->val);
            peer_window_size -= p->val.length_in_sequence_space();
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return static_cast<unsigned int>(_timer.fail_count()); }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    auto header = seg.header();
    header.seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}
