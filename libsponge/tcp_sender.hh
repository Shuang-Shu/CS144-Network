#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "timer.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <memory>
#include <queue>

using namespace std;

template <class T>
class Node {
  public:
    T val;
    shared_ptr<Node> next;
    shared_ptr<Node> prev;
    Node(T v) : val(v), next(nullptr), prev(nullptr) {}
};

template <class T>
class LinkedBuffer {
  public:
    shared_ptr<Node<T>> head;
    shared_ptr<Node<T>> tail;
    void addTail(T);
    void removeTail();
    T pop();
    T peek();
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
    LinkedBuffer<TCPSegment> _outstanding_buffer;

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
    uint16_t peer_win() const { return _peer_window_size; }

    //! \brief return the continous failed number of the timer
    uint16_t fail_count() { return _timer.fail_count(); }

    //! \brief
    bool fin_acked(uint64_t fin_next_seqno){
      return _acked_next>=fin_next_seqno;
    }
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
