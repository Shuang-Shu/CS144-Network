#include "tcp_receiver.hh"
#include <algorithm>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    //DUMMY_CODE(seg);
    start=start|seg.header().syn;
    bool eof=false;
    if(!start)
        return;
    if(seg.header().syn==true)
        isn=seg.header().seqno;
    size_t absSeqNo=unwrap(seg.header().seqno, isn, _reassembler.getExpectedIdx());
    if(seg.header().fin==true){
        endIndex=absSeqNo+seg.payload().size();
        if(seg.header().syn==true)
            endIndex+=1;
        eof=true;
    }
    auto streamIdx=absSeqNo-1;
    if(seg.header().syn==true)
        streamIdx=absSeqNo;
    auto load=seg.payload().copy();
    load=load;
    _reassembler.push_substring(seg.payload().copy(), streamIdx, eof);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(!start)
        // 若尚未遇到SYN
        return std::nullopt;
    else{
        auto index=_reassembler.getExpectedIdx();
        if(index+1==endIndex)
            return wrap(index+2, isn);
        return wrap(index+1, isn);
    }
}

size_t TCPReceiver::window_size() const {
    return _capacity-_reassembler.streamSize();
 }

