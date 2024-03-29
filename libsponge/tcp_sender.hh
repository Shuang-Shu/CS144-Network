#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <memory>
#include <queue>
#include <deque>

using namespace std;

class Timer {
  private:
    uint64_t _rto;
    uint64_t _initial_rto;
    uint64_t _time_pass;
    uint16_t _fail_count;
    bool _running;
    // shift of _initial_retransmission_timeout
    // current_isn=_initial_retransmission_timeout<<_retransmission_timeout_shift
    unsigned int _retransmission_timeout_shift{0};

  public:
    Timer(uint64_t _init_rto)
        : _rto(_init_rto)
        , _initial_rto(_init_rto)
        , _time_pass(0)
        , _fail_count(0)
        , _running(false)
        , _retransmission_timeout_shift(0) {}

    // call this function to change time
    void pass(uint64_t);
    // stop the timer
    void stop();
    // start the timer
    void start();
    // reset the timer
    void reset();
    // check whether the timer is expire
    bool is_expire();
    // get the continous fail count
    uint16_t fail_count() const;
    // double the rto
    void double_rto();
    // is this timer running
    bool is_running() { return _running; }
    // reset the rto
    void reset_rto();
};

template <class T>
class Node {
  public:
    T val;
    shared_ptr<Node> next;
    shared_ptr<Node> prev;
    Node(T v) : val(v), next(nullptr), prev(nullptr) {}
};

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    //! retransmission timer for the connection
    unsigned int _initial_retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0};

    //! the next (absolute) sequence number for the first acked byte
    uint64_t _acked_next{0};

    //! timer used to log time
    Timer _timer{_initial_retransmission_timeout};

    //! linked list of outstanding segment
    deque<TCPSegment> _outstanding_buffer;

    //! current peer window size
    uint16_t _peer_window_size{1};

    // newest received window_size from ack
    uint64_t _received_window_size{1};

    //! has SYN segment sent
    bool _syn_sent{false};

    //! has FIN segment sent
    bool _fin_sent{false};

    //! bytes sent but not acked yet
    uint64_t _bytes_in_flight{0};

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}

    //! \brief the continous failed number of expiration
    uint64_t fail_count() { return _timer.fail_count(); }

    // just for test
    bool sender_isn_has_value() { return _isn.raw_value() != 0; }

    WrappingInt32 isn() const { return _isn; }
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
