#include "tcp_sender.hh"

#include "tcp_config.hh"
#include "wrapping_integers.hh"
#include "timer.hh"
#include <random>
#include <utility>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    ,_initial_retransmission_timeout{retx_timeout}
    , _stream(capacity), outstandingSegBuf(), timer(retx_timeout), peerWindowSize(1), reachedLastSeg(false), RTO(retx_timeout) {}

// 参考TCPSegment::length_in_sequence_space
/*
 * 需要记录已发送但未确认的TCP报文字节数
 * /*/
uint64_t TCPSender::bytes_in_flight() const {
    size_t result=0;
    for(auto iter=outstandingSegBuf.begin();iter!=outstandingSegBuf.end();++iter)
        result+=(*iter).second.length_in_sequence_space();
    return result; 
}

void TCPSender::fill_window() {
    // 最大的报文长度
    size_t maxSegSize=TCPConfig::MAX_PAYLOAD_SIZE;
    // 剩余的窗口尺寸
    size_t leftWindowSize=peerWindowSize-bytes_in_flight();
    // 这里的maxSegSize已经排除了SYN和FIN所占用的空间
    maxSegSize=min(maxSegSize, leftWindowSize);

    string payload=_stream.read(maxSegSize);
    // 利用std::move()函数，可以将右值引用绑定到左值
    Buffer buffer(std::move(payload));
    // 生成TCPHeader
    // 只关心 sqeno,SYN,FIN,Payload四个部分的填写
    TCPHeader header;
    if(_next_seqno==0)
        // 若绝对seqno为0，则为第一个发送的Segment
        header.syn=true;
    if(_stream.eof()){
        // 此时输入流已空且输入已经结束，因此标记FIN flag
        header.fin=true;
        if(reachedLastSeg){
            // 如果已经到达了最后一个Seg，则不再新建空的FIN报文
            return;
        }
        reachedLastSeg=true;
    }
    // 更新_next_seqno
    _next_seqno+=maxSegSize;
    // 组合TCP报文
    TCPSegment seg0;
    auto segHeader=seg0.header();
    auto segLoad=seg0.payload();
    segHeader=header;
    segLoad=buffer;
    // 发送TCP报文
    _segments_out.push(seg0);
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
// 收到接收方的ack后的反应
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    // 更新窗口大小
    peerWindowSize=window_size;
    // 更新outstandingSegBuf
    // 1. 将ackno解包为绝对序号
    map<size_t, TCPSegment>::iterator iter=outstandingSegBuf.begin();
    size_t checkpoint=(*iter).first;
    uint64_t absSeq=unwrap(ackno, _isn, checkpoint);
    // 2. 删除所有已确认的TCP报文段
    while((*iter).first+(*iter).second.length_in_sequence_space()<=absSeq){
        auto temp=iter;
        iter++;
        outstandingSegBuf.erase(temp);
    }
    // 3. 重设计时器
    timer.setRTO(_initial_retransmission_timeout);
    timer.reset();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
// 这个方法被上层调用，其参数为其距离上一次调用经过的时间(ms)
void TCPSender::tick(const size_t ms_since_last_tick) { 
    // DUMMY_CODE(ms_since_last_tick); 
    // 更新计时器
    timer.addTime(ms_since_last_tick);
    if(timer.isEtire()){
        // 若已经超时
        // 1.重传
        consecutive_retransmissions();
        // 2.增大2倍RTO
        RTO*=2;
        // 3.重设Timer
        timer.setRTO(RTO);
        timer.reset();
        // 判断是否连续重传，可以检查Timer的RTO，若其不是_initial_retransmission_timeout，则说明其在进行连续重传
    }
    else
        // 若未超时，
        fill_window();
}

unsigned int TCPSender::consecutive_retransmissions() const { 
    return {}; 
}

void TCPSender::send_empty_segment() {
    
}
