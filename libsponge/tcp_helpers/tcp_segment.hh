#ifndef SPONGE_LIBSPONGE_TCP_SEGMENT_HH
#define SPONGE_LIBSPONGE_TCP_SEGMENT_HH

#include "buffer.hh"
#include "tcp_header.hh"

#include <cstdint>

//! \brief [TCP](\ref rfc::rfc793) segment
class TCPSegment {
  private:
    // TCP报文头部
    TCPHeader _header{};
    // 有效荷载
    Buffer _payload{};

  public:
    //! \brief Parse the segment from a string
    // 从一个字符串中解析报文
    ParseResult parse(const Buffer buffer, const uint32_t datagram_layer_checksum = 0);

    //! \brief Serialize the segment to a string
    // 将报文序列号为一个字符串
    BufferList serialize(const uint32_t datagram_layer_checksum = 0) const;

    //! \name Accessors
    //!@{
    const TCPHeader &header() const { return _header; }
    TCPHeader &header() { return _header; }

    const Buffer &payload() const { return _payload; }
    Buffer &payload() { return _payload; }
    //!@}

    //! \brief Segment's length in sequence space
    //! \note Equal to payload length plus one byte if SYN is set, plus one byte if FIN is set
    size_t length_in_sequence_space() const;
};

#endif  // SPONGE_LIBSPONGE_TCP_SEGMENT_HH
