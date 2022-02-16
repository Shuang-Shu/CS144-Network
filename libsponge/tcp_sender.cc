#include "tcp_sender.hh"
#include "wrapping_integers.hh"
#include "tcp_config.hh"

#include <random>
#include <cmath>
#include <map>
#include <set>

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
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , outstandingSegs()
    , timer(_initial_retransmission_timeout) {}

// ----------------------------------------
uint64_t TCPSender::bytes_in_flight() const { 
    return {}; 
}

// ----------------------------------------
/*
    生成TCP报文
*/
TCPSegment TCPSender::getSegment(string payload, TCPHeader header){
    TCPSegment segment;
    segment.header()=header;
    Buffer buffer=Buffer(std::move(payload));
    segment.payload()=buffer;
    return segment;
}

// ----------------------------------------
/*
    生成Header，Header中只有下述值需要调整:
        1. seqNo
        2. SYN
        3. FIN
*/
TCPHeader TCPSender::getHeader(size_t absSeqNo, bool eof){
    TCPHeader header;
    // 确定seqNo
    WrappingInt32 wrappingInt32=wrap(absSeqNo, _isn);
    header.seqno=wrappingInt32;
    // 确定SYN
    if(absSeqNo==0)
        header.syn=true;
    // 确定FIN
    if(eof)
        header.fin=true;
    return header;
}

// ----------------------------------------
void TCPSender::fill_window() {
    // 获取seg的可能数量
    size_t segNum=windowSize/TCPConfig::MAX_PAYLOAD_SIZE;
    if(windowSize%TCPConfig::MAX_PAYLOAD_SIZE!=0)segNum+=1;
    for(size_t i=0;i<segNum;++i){
        string payload=_stream.read(min(windowSize, TCPConfig::MAX_PAYLOAD_SIZE));
        // 此处absSeqNo的计算存在问题（未考虑FIN）
        size_t absSeqNo=_stream.bytes_read()+1;
        // 考虑不含payload只有FIN的报文
        if(payload.length()==0&&_stream.eof())
            absSeqNo+=1;
        if(_stream.bytes_read()==0)
            absSeqNo=0;
        _next_seqno=absSeqNo+payload.length();
        TCPHeader header=getHeader(absSeqNo, _stream.eof());
        TCPSegment segment=getSegment(payload, header);
        // 1 将输入流改为多个TCPSegment并发送
        _segments_out.push(segment);
        if(!timer.isRunning())
            timer.open();
        // 2 上述报文在需要进行缓存（键为absSeqNo）
        outstandingSegs[absSeqNo]=segment;
    }
}

// ----------------------------------------
//! \param ackno The remote receiver's ackno (acknowledgment number)
// 远程接收器的确认号(确认号)
//! \param window_size The remote receiver's advertised window size
// 远程接收器建议的窗口大小
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // 更新对等方的windowSize数据
    windowSize=window_size>1?window_size:1;
    // 更新自身的确认号
    ackedAbsSeqNo=unwrap(ackno, _isn, ackedAbsSeqNo);
    // 删除缓存中已经确认的报文
    set<size_t> deleteSegments;
    for(map<size_t, TCPSegment>::iterator iter=outstandingSegs.begin();iter!=outstandingSegs.end();++iter){
        if((*iter).first+(*iter).second.length_in_sequence_space()<=ackedAbsSeqNo){
            // 删除受确认的报文段
            deleteSegments.insert((*iter).first);
            // 重设计时器
            timer.reset();
        }
    }
    // 删除已经被确认的segments
    for(auto iter=deleteSegments.begin();
    iter!=deleteSegments.end();++iter)
        outstandingSegs.erase((*iter));
    if(outstandingSegs.empty())
        timer.close();
}

// ----------------------------------------
//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
// 自上次调用此方法以来的毫秒数
void TCPSender::tick(const size_t ms_since_last_tick) { 
    timer.addTime(ms_since_last_tick);
    if(!timer.isEtire()){
        // 若未超时，不进行任何操作
    }else if(windowSize!=0){
        // 若已经超时，且接收窗口大小不为0，重传所有突出报文段
        for(auto iter=outstandingSegs.begin();iter!=outstandingSegs.end();++iter)
            _segments_out.push((*iter).second);
        // 重设计时器，同时将RTO翻倍
        timer.doubleRTO();
        timer.restart();
    }
}

// ----------------------------------------
unsigned int TCPSender::consecutive_retransmissions() const { 
    return timer.getRetransCount();
}

// ----------------------------------------
// 发送一个占用absSeqNo为0的报文，它主要用于接收方发送ackNo
void TCPSender::send_empty_segment() {
    size_t absSeqNo=_stream.bytes_read()+1;
    // 考虑不含payload只有FIN的报文
    if(_stream.eof())
        absSeqNo+=1;
    TCPHeader header=getHeader(absSeqNo, _stream.eof());
    TCPSegment segment=getSegment("", header);
}
