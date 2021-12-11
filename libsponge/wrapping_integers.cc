#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint64_t temp=n+isn.raw_value();
    uint32_t result=temp;
    return WrappingInt32(result);
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    //DUMMY_CODE(n, isn, checkpoint);
    auto nRaw=n.raw_value();
    auto isnRaw=isn.raw_value();
    auto dist=nRaw-isnRaw;
    uint64_t temp=1;
    temp=temp<<32;
    uint64_t offset=checkpoint%temp;
    uint64_t base=(checkpoint/temp)*temp;
    uint64_t result,temp1,temp2;
    if(offset<dist){
        temp1=base-temp+dist;
        temp2=base+dist;
        if(absDist(temp1, checkpoint)>=absDist(temp2, checkpoint))
            result=temp2;
        else
            result=temp1;
    }
    else if(offset==dist)
        result=base+dist;
    else{
        temp1=base+temp+dist;
        temp2=base+dist;
        if(absDist(temp1, checkpoint)>=absDist(temp2, checkpoint))
            result=temp2;
        else
            result=temp1;
    }
    return result;
}

uint64_t absDist(uint64_t a, uint64_t b){
    if(a>=b)
        return a-b;
    else
        return b-a;
}