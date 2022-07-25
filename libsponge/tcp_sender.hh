#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"
#include "../tcp_helpers/tcp_segment.hh"
#include "timer.hh"

#include <functional>
#include <map>
#include <queue>

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    // 我们的初始序列号，SYN报文的数字
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    // TCP发送器希望发送报文的输出队列
    std::queue<TCPSegment> _segments_out{};

    //! retransmission timer for the connection
    // 初始重传时间，这个值不变
    unsigned int _initial_retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    // 尚未被发送的上层字节流
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    // 下一个要发送的字节的绝对seqNo
    uint64_t _next_seqno{0};
    // 收到确认的absSeqNo
    uint64_t _acked_seqno{0};
    // 新定义成员变量
    // 未确认报文
    map<size_t, TCPSegment> outstandingSegBuf{};
    // 计时器
    Timer timer;
    // 对等方剩余窗口大小(bytes)这是用于发送的
    uint16_t peerWindowSize;
    // 对等方的真实windowSize，可能为0
    uint16_t real_window_size{1};
    // 是否到达最后一个segment
    bool sent_last_segment;
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

    // 用于发送单独的ack报文
    void send_ack_segment(WrappingInt32 ackno, size_t window_size); 

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
};
#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH