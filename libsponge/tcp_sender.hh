#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"
#include "timer.hh"

#include <functional>
#include <queue>
#include <map>

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! 我们的初始序列号，即SYN的序列号。
    WrappingInt32 _isn;

    //! TCPSender想要发送的报文的输出队列
    std::queue<TCPSegment> _segments_out{};

    //! 连接的重传计时器
    unsigned int _initial_retransmission_timeout;

    //! 尚未发送的输出字节流
    ByteStream _stream;

    //! 要发送的下一个字节的(绝对)序列号，可以利用_stream的bytes_read()函数返回值进行计算
    uint64_t _next_seqno{0};

    // 对等方的接收窗口大小
    uint64_t windowSize{1};

    // 存储突出报文的Map，键为对应报文的absSqeNo
    std::map<size_t, TCPSegment> outstandingSegs;

    // 存储当前已被确认的absSeqNo
    size_t ackedAbsSeqNo{0};

    // 一个计时器
    Timer timer;
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
    // ----------------------------------------
    // 自定函数
    // 根据报文头和payload生成TCPSegment
    TCPSegment getSegment(string, TCPHeader);
    /* 
      生成Header，Header中只有下述值需要调整:
        1. seqNo
        2. SYN
        3. FIN
    */
    TCPHeader getHeader(size_t, bool);
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
