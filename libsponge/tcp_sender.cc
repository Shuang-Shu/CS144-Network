#include "tcp_sender.hh"

#include "tcp_config.hh"
#include "wrapping_integers.hh"
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
    , _stream(capacity), outstandingSegBuf(), timer(retx_timeout), peerWindowSize(1), sent_last_segment(false) {}

// 参考TCPSegment::length_in_sequence_space
/*
 * 需要记录已发送但未确认的TCP报文字节数
 * /*/
uint64_t TCPSender::bytes_in_flight() const {
    size_t result=0;
    result=_next_seqno-_acked_seqno;
    return result; 
}

void TCPSender::fill_window() {
    for(;;){
        // 最大的报文长度
        size_t maxSegSize=TCPConfig::MAX_PAYLOAD_SIZE;
        // 剩余的窗口尺寸
        size_t leftWindowSize=peerWindowSize-bytes_in_flight();
        // 这里的maxSegSize已经排除了SYN和FIN所占用的空间
        maxSegSize=min(maxSegSize, leftWindowSize);
        if(maxSegSize==0)
            return; // 此时接收方窗口已满
        string payload=_stream.read(maxSegSize);
        uint64_t payload_length=payload.length();
        size_t old_next_seqno=_next_seqno;
        if(payload_length==0&&_next_seqno!=0&&!_stream.eof())
            return; // 此时未从流中读取信息，不发送新报文
        // 利用std::move()函数，可以将右值引用绑定到左值
        Buffer buffer(std::move(payload));
        // 生成TCPHeader
        // 组合TCP报文
        TCPSegment seg0;
        // 只关心 sqeno,SYN,FIN,Payload四个部分的填写
        TCPHeader &header=seg0.header();
        header.seqno=wrap(_next_seqno, _isn);
        if(_next_seqno==0){
            // 若绝对seqno为0，则为第一个发送的Segment
            header.syn=true;
            _next_seqno+=1;
        }
        if(_stream.eof()&&payload_length<leftWindowSize){
            // 此时输入流已空且输入已经结束，因此标记FIN flag
            header.fin=true;
            if(sent_last_segment){
                // 如果已经到达了最后一个Seg，则不再新建空的FIN报文
                return;
            }
            _next_seqno++;// 这是FIN的占位
            sent_last_segment=true;
        }
        // 更新_next_seqno
        _next_seqno+=payload_length;
        
        auto &segLoad=seg0.payload();
        segLoad=buffer;
        // 将报文加入outstandingSegBuf
        outstandingSegBuf[old_next_seqno]=seg0;
        // 发送TCP报文
        _segments_out.push(seg0);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
// 收到接收方的ack后的反应
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    // 更新窗口大小
    peerWindowSize=window_size;
    real_window_size=window_size;
    if(window_size==0)
        peerWindowSize=1;
    // 更新outstandingSegBuf
    // 1. 将ackno解包为绝对序号
    uint64_t absSeq=unwrap(ackno, _isn, _acked_seqno);
    // bool ack_has_new_data=true;
    if(absSeq<=_acked_seqno)
        return;
    if(absSeq>_next_seqno)
        return; // 不可能的确认号被忽略
    _acked_seqno=absSeq;
    if(outstandingSegBuf.size()==0)
        return;
    map<size_t, TCPSegment>::iterator iter=outstandingSegBuf.begin();
    // 2. 删除所有已确认的TCP报文段
    for(size_t i=0;i<outstandingSegBuf.size();++i){
        uint64_t target_seq_no=(*iter).first+(*iter).second.length_in_sequence_space();
        // if((*iter).second.header().fin==true)
        //     target_seq_no--;
        if(target_seq_no>_acked_seqno) break;
        auto temp=iter;
        iter++;
        outstandingSegBuf.erase(temp);
    }
    // 3. 重设计时器
    timer.reset(_initial_retransmission_timeout);
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
// 这个方法被上层调用，其参数为其距离上一次调用经过的时间(ms)
void TCPSender::tick(const size_t ms_since_last_tick) { 

    // 更新计时器
    timer.add_time(ms_since_last_tick);
    if(timer.timeout()){
        // 重传（按照文档的要求，一次只重传一个segment）
        if(outstandingSegBuf.size()==0)
            return;
        auto iter=outstandingSegBuf.begin();
        _segments_out.push((*iter).second);
        // for(auto iter:outstandingSegBuf){
        //     _segments_out.push(iter.second);
        //     break;
        // }
        // 提高连续重传次数
        timer.increase_retran_number();
        // 增大rto，并重设time_passed
        if(real_window_size!=0)
            timer.double_rto();
        else
            timer.reset_passed_time();
    }else{
        fill_window();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { 
    return timer.get_retran_number(); 
}

// 发送一个空报文段
void TCPSender::send_empty_segment(){
    _segments_out.push(TCPSegment());
}

void TCPSender::send_ack_segment(WrappingInt32 ackno, size_t window_size) {
    TCPSegment seg;
    // 只关心 sqeno,SYN,FIN,Payload四个部分的填写
    TCPHeader &header=seg.header();
    header.ack=true;
    header.ackno=ackno;
    header.win=window_size;
    _segments_out.push(seg);
}

bool TCPSender::timeout(){
    return timer.timeout();
}