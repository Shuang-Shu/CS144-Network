#ifndef SPONGE_LIBSPONGE_TCP_FACTORED_HH
#define SPONGE_LIBSPONGE_TCP_FACTORED_HH

#include "tcp_config.hh"
#include "tcp_receiver.hh"
#include "tcp_sender.hh"
#include "tcp_state.hh"

//! \brief A complete endpoint of a TCP connection
// TCP连接的一个完整端点
class TCPConnection {
  private:
    TCPConfig _cfg;
    // 这里使用了C++ 11 的通过大括号进行初始化的方法
    // 相当于 TCPReceiver _receiver=TCPReceiver(_cfg.recv_capacity);
    TCPReceiver _receiver{_cfg.recv_capacity};
    TCPSender _sender{_cfg.send_capacity, _cfg.rt_timeout, _cfg.fixed_isn};

    //! outbound queue of segments that the TCPConnection wants sent
    // TCPConnection希望发送的段的出站队列
    std::queue<TCPSegment> _segments_out{};

    //! Should the TCPConnection stay active (and keep ACKing)
    //! for 10 * _cfg.rt_timeout milliseconds after both streams have ended,
    //! in case the remote TCPConnection doesn't know we've received its whole stream?
    // 此TCPConnection是否在两个流已经结束后保持活动(并保持跟踪)10 * _cfg.rt_timeout毫秒，以防远程TCPConnection没有收到我们对它的整个流的确认
    bool _linger_after_streams_finish{true};
    // 自从上次收到segment后经历的时间
    size_t ms_since_last_segment_received{0};
    // 连接是否仍然活动
    bool isActive{true};
    // 4个干净结束流的先决条件
    bool inboundReassembled{false};
    bool outboundFinished{false};
    bool outboundAckedByPeer{false};
    // 最后一个先决条件通过徘徊或直接结束来确认
    bool finished{false};
    WrappingInt32 finAckno{0};

  public:
    //! \name "Input" interface for the writer
    //!@{

    //! \brief Initiate a connection by sending a SYN segment
    // 通过发送一个SYN来初始化连接
    void connect();

    //! \brief Write data to the outbound byte stream, and send it over TCP if possible
    // 将数据写入输出流，如果可能，将其通过TCP发送
    //! \returns the number of bytes from `data` that were actually written.
    size_t write(const std::string &data);

    //! \returns the number of `bytes` that can be written right now.
    // 返回当前可以被写入的“字节”数
    size_t remaining_outbound_capacity() const;

    //! \brief Shut down the outbound byte stream (still allows reading incoming data)
    void end_input_stream();
    //!@}

    //! \name "Output" interface for the reader
    //!@{

    //! \brief The inbound byte stream received from the peer
    ByteStream &inbound_stream() { return _receiver.stream_out(); }
    //!@}

    //! \name Accessors used for testing

    //!@{
    //! \brief number of bytes sent and not yet acknowledged, counting SYN/FIN each as one byte
    // 已发送但尚未确认的字节数，将SYN/FIN每个字节计算为一个字节
    size_t bytes_in_flight() const;
    //! \brief number of bytes not yet reassembled
    // 尚未重组的字节数
    size_t unassembled_bytes() const;
    //! \brief Number of milliseconds since the last segment was received
    // 从收到最后一个片段到现在的毫秒数
    size_t time_since_last_segment_received() const;
    //!< \brief summarize the state of the sender, receiver, and the connection
    // 总结发送方、接收方和连接的状态
    TCPState state() const { return {_sender, _receiver, active(), _linger_after_streams_finish}; };
    //!@}

    //! \name Methods for the owner or operating system to call
    //!@{

    //! Called when a new segment has been received from the network
    // 当从网络接收到一个新的segment时调用
    void segment_received(const TCPSegment &seg);

    //! Called periodically when time elapses
    void tick(const size_t ms_since_last_tick);

    //! \brief TCPSegments that the TCPConnection has enqueued for transmission.
    //! \note The owner or operating system will dequeue these and
    //! put each one into the payload of a lower-layer datagram (usually Internet datagrams (IP),
    //! but could also be user datagrams (UDP) or any other kind).
    std::queue<TCPSegment> &segments_out() { return _segments_out; }

    //! \brief Is the connection still alive in any way?
    // 连接还存在吗?
    //! \returns `true` if either stream is still running or if the TCPConnection is lingering
    //! after both streams have finished (e.g. to ACK retransmissions from the peer)
    bool active() const;
    //!@}

    //! Construct a new connection from a configuration
    explicit TCPConnection(const TCPConfig &cfg) : _cfg{cfg} {}

    //! \name construction and destruction
    //! moving is allowed; copying is disallowed; default construction not possible

    //!@{
    ~TCPConnection();  //!< destructor sends a RST if the connection is still open
    TCPConnection() = delete;
    TCPConnection(TCPConnection &&other) = default;
    TCPConnection &operator=(TCPConnection &&other) = default;
    TCPConnection(const TCPConnection &other) = delete;
    TCPConnection &operator=(const TCPConnection &other) = delete;
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_FACTORED_HH
