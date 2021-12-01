#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <iostream>

using namespace std;

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.
    size_t capacity;    //!< The maximum number of bytes
    ByteStream _output;  //!< The reassembled in-order byte stream
    map<size_t, string> unassembledBuf; // 未重组的缓冲区
    vector<size_t> idx_heap; // 索引的小顶堆
    string assembledBuf; // 已经重组的缓冲区
    size_t unassembledSize; // 未重组的大小
    size_t expectedIdx; // 期望的索引，指相对于已重组的部分
    bool eof;

    // 方法区
    // 将新的string插入到unassembledBuf中
    void insertStr(const string &data, size_t index);
    // 将unassembledBuf中的str尽可能多地重组为有序
    void assembleStr();
    // 将assembledBuf中字节写入到_output中
    void writeStr(); 
    // 返回空间是否已满
    bool isFull();
    // 将新的data推入
    void pushData(const string &data, const size_t &index);
    // 检测重叠str
    map<size_t, string>::iterator detectOverlap(size_t index, const string &data, size_t &leftOverlap, size_t &rightOverlap, size_t &mergeLength, size_t &oldLength);
    // 合并substr
    string merge(map<size_t, string>::iterator iter, size_t overlapNum, size_t mergedLength);

    // 堆运算
    // 插入元素到heap中
    void insertToHeap(size_t);
    // 获取堆顶元素
    size_t peek();
    // 弹出堆顶元素
    size_t pop();

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
};

void showMap(map<size_t, string> map0);

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
