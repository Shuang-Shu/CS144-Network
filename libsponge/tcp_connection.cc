#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { 
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const { 
    return _sender.bytes_in_flight(); 
}

size_t TCPConnection::unassembled_bytes() const { 
    return _receiver.unassembled_bytes(); 
}

size_t TCPConnection::time_since_last_segment_received() const { 
    return ms_since_last_segment_received;
}

void TCPConnection::segment_received(const TCPSegment &seg) { 
    ms_since_last_segment_received=0;
    if(seg.header().rst){
        // 此时reset被置位
        // 是第一种TCPConnection结束的情况
        // 1. 将入站和出站流都设置为**错误**的状态
        _sender.stream_in().error();
        _receiver.stream_out().error();
        // 2. 永久地终止连接（未完成）
        isActive=false;
    }else{
        // 将收到的segment交给receiver
        _receiver.segment_received(seg);
        // 确认输入的segment是否都已经被组装，且inbound流已经结束
        if(_receiver.unassembled_bytes()==0&&_receiver.stream_out().eof()==true)
            inboundReassembled=true;
        // FIN的报文是否已经被确认？
        if(finished and seg.header().ackno.raw_value()>=finAckno.raw_value())
            outboundAckedByPeer=true;
        // 如果输入流已经被关闭，将linger设置为false
        if(_receiver.stream_out().eof())
            _linger_after_streams_finish=false;
        // 如果segment中的ack被置位，将相关信息告知_sender
        if(seg.header().ack){
            _sender.ack_received(seg.header().ackno, seg.header().win);
        }
        // 如果收到的segment占用了序列空间，回复一个segment反应当前的ackno和窗口大小
        if(seg.length_in_sequence_space()>0)
            _sender.send_ack_segment(_receiver.ackno().value(), _receiver.window_size());
        // 
        if ((_receiver.ackno().has_value() && (seg.length_in_sequence_space() == 0) and (seg.header().seqno == _receiver.ackno().value() - 1))) {
            _sender.send_empty_segment();
        }
    }
    // DUMMY_CODE(seg); 
}

bool TCPConnection::active() const { 
    return isActive; 
}

size_t TCPConnection::write(const string &data) {
    // 将data写入_sender的输出流
    size_t written=_sender.stream_in().write(data);
    // _sender.fill_window();
    // 将TCPSender中的Segments**全部**弹出，并使用TCPReceiver的信息进行处理，然后发送
    bool hasAckno=false;
    WrappingInt32 ackno(0);
    if(_receiver.ackno().has_value()){
        hasAckno=true;
        ackno=_receiver.ackno().value();
    } 
    while (!_sender.segments_out().empty()){
        TCPSegment &seg=_sender.segments_out().front();
        _sender.segments_out().pop();
        if(hasAckno){
            // 如果有一个ackno，它将设置TCPSegment中的`ACK`位
            seg.header().ackno=ackno;
            seg.header().ack=true;
        }
        if(seg.header().fin){
            finished=true;
            finAckno=seg.header().ackno;
        }
        _segments_out.push(seg);
        if(_sender.stream_in().eof())
            outboundFinished=true;
    }
    // DUMMY_CODE(data);
    return written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {     
    // DUMMY_CODE(ms_since_last_tick); 
    ms_since_last_segment_received+=ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    // 在ms_since_last_segment_received达到某个值后，需要“干净的”结束连接
    if(inboundReassembled and outboundFinished and outboundAckedByPeer){
        if(!_linger_after_streams_finish){
            // 此时不需要徘徊
            isActive=false;
        }
        if(ms_since_last_segment_received>=10*_cfg.rt_timeout){
            // 徘徊时间超过了10*_cfg.rt_timeout
            isActive=false;
        }
    }
    
    // if(ms_since_last_segment_received>)
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
}

void TCPConnection::connect() {
    TCPSegment synSeg{};
    TCPHeader &header=synSeg.header();
    header.syn=true;
    // 发送synSeg的工作由TCPConnection完成
    _segments_out.push(synSeg);
}

// 析构函数
TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            // Your code here: need to send a RST segment to the peer
            TCPSegment segment;
            segment.header().rst=true;
            _segments_out.push(segment);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
