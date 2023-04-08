#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <string>
#include <queue>

class CycleArray {
  private:
    size_t _capacity;
    size_t _head; // head of queue
    // size_t _tail; // tail of queue(not refer to a element)
    size_t _head_index_in_stream;
    char *_array;
    bool *_null_array;
    // functions

    // convert login index to real index
    size_t _logic_index_to_real(size_t logic_index);
    // convert real index to logic
    size_t _real_index_to_logic(size_t real_index); 

  public:
    CycleArray(size_t cap);
    // return the char at logic index
    char get_at_index(size_t);
    // return is nall at logic index
    bool get_is_null(size_t);
    void set_index_null(size_t);
    // set char at logic index when there is zero
    bool set_at_index_zero(size_t l_index, char c);
    // set value of element to 0 from l_index, until meet 0, then reset _head
    char pop_head(); 
    size_t peek_head_length(); 
    size_t get_head_index_in_stream(); 
};

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.

    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes
    CycleArray _unassembled; // unassesbled buffer
    std::queue<char> _assembled; // assembled char stream of input
    size_t _assembled_bytes;  // bytes of assembled bytes
    size_t _unassembled_bytes; // bytes of unassembled bytes
    size_t _expect; // index of closest unreceived byte
    size_t _eof_index; // index which is behind last char
    static const int CHAR_LENGTH=1;
  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;

    // push chars in _assembled to _output
    bool push_to_stream();
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
