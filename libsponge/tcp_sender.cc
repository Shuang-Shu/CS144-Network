#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <iostream>
#include <memory>
#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

using namespace std;

void Timer::pass(uint64_t ms) {
    if (!_running) {
        return;
    }
    _time_pass += ms;
}

void Timer::stop() {
    reset();
    _running = false;
}

void Timer::start() { _running = true; }

void Timer::reset() {
    _time_pass = 0;
    _fail_count = 0;
    _retransmission_timeout_shift = 0;
    _rto = _initial_rto;
}

bool Timer::is_expire() { return _time_pass >= _rto; }

uint16_t Timer::fail_count() const { return _fail_count; }

void Timer::double_rto() {
    _fail_count++;
    _time_pass = 0;
    _retransmission_timeout_shift++;
    _rto = _initial_rto << _retransmission_timeout_shift;
}

void Timer::reset_rto() {
    _retransmission_timeout_shift = 0;
    _rto = _initial_rto;
    _time_pass = 0;
}

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

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

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
    while ((!_fin_sent && stream_in().eof()) || !_syn_sent ||
           (data = get_data(stream_in(), min(static_cast<uint16_t>(TCPConfig::MAX_PAYLOAD_SIZE), _peer_window_size)))
                   .length() != 0) {
        TCPSegment seg;
        seg.payload() = Buffer(move(data));
        TCPHeader &header = seg.header();
        header.seqno = wrap(_next_seqno, _isn);
        if (!_syn_sent) {
            // set SYN flag
            header.syn = true;
            _syn_sent = true;
        }
        if (stream_in().eof()) {
            header.fin = true;
            _fin_sent = true;
        }
        auto len_in_seq_space = seg.length_in_sequence_space();
        bool directly_break = false;
        if (len_in_seq_space > _peer_window_size) {
            header.fin = false;
            _fin_sent = false;
            directly_break = true;
            len_in_seq_space = seg.length_in_sequence_space();
        }
        if (len_in_seq_space == 0)
            return;
        _segments_out.push(seg);
        _next_seqno += len_in_seq_space;
        _bytes_in_flight += len_in_seq_space;
        if (_peer_window_size >= len_in_seq_space)
            _peer_window_size -= len_in_seq_space;
        else
            _peer_window_size = 0;
        if (!_timer.is_running())
            _timer.start();
        _outstanding_buffer.push_back(seg);
        if (directly_break)
            break;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // cerr << "WIN MY DEBUG: peer win=" << window_size << endl;
    auto window_size_copy = window_size;
    _received_window_size = window_size;
    if (window_size == 0) {
        window_size_copy = 1;
        // when window_size is ZERO, the RTO of timer should not be increased
    }
    auto abs_ackno = unwrap(ackno, _isn, _acked_next);
    if (abs_ackno > _next_seqno) {
        // ignore impossible ackno
        return;
    }
    // remove outstanding segments in the buffer
    bool new_data = false;
    while (true) {
        if (_outstanding_buffer.empty()) {
            break;
        }
        TCPSegment &head_seg = _outstanding_buffer.front();
        auto length_in_seq = head_seg.length_in_sequence_space();
        auto head_seg_seqno = unwrap(head_seg.header().seqno, _isn, _acked_next);
        if (abs_ackno > head_seg_seqno) {
            new_data = true;
            if (abs_ackno - head_seg_seqno >= length_in_seq) {
                // cerr << "MY DEBUG: acked seqno=" << ackno << endl;
                _bytes_in_flight -= length_in_seq;
                _acked_next += length_in_seq;
                _outstanding_buffer.pop_front();
            } else {
                break;
            }
        } else {
            break;
        }
    }
    // if the receiver anounced that its window size is ZERO,
    // the _peer_window_size should be ONE
    if (window_size_copy > _bytes_in_flight)
        _peer_window_size = window_size_copy - _bytes_in_flight;
    else
        _peer_window_size = 0;
    // reset the timer
    if (_outstanding_buffer.empty()) {
        _timer.stop();
    }
    if (new_data) {
        _timer.reset();
    }
    // when received a ack, should fill peer's window
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer.pass(ms_since_last_tick);
    if (_timer.is_expire()) {
        // cerr << "DEBUG: timer has expired!\n";
        if (_received_window_size != 0)
            _timer.double_rto();
        else
            _timer.reset_rto();
        if (!_outstanding_buffer.empty()) {
            _segments_out.push(_outstanding_buffer.front());
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return static_cast<unsigned int>(_timer.fail_count()); }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}
