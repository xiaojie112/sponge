#include "tcp_connection.hh"
#include <cassert>
#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return time_pass - segment_last_received_time; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    segment_last_received_time = time_pass;
    if(seg.header().rst){
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        return;
    }
    _receiver.segment_received(seg);

    if(seg.header().ack){
        // cout << "test5" << endl;
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }
    
    //closed  -> listen
    if(TCPState::state_summary(_receiver) == TCPReceiverStateSummary::SYN_RECV
    && TCPState::state_summary(_sender) == TCPSenderStateSummary::CLOSED){
        connect();
        return;
    }

//     if the incoming segment occupied any sequence numbers, the TCPConnection makes
// sure that at least one segment is sent in reply, to reflect an update in the ackno and
// window size.
//TODO: 重复发送空的ack虽然不占用序列空间，但是可能影响性能
    if(seg.length_in_sequence_space() != 0)_sender.send_empty_segment();


    //respond to keep-alive segment
    if(_receiver.ackno().has_value() && (seg.length_in_sequence_space() == 0) && seg.header().seqno == _receiver.ackno().value() - 1){
        _sender.send_empty_segment();
    }

    // 判断 TCP 断开连接时是否时需要等待
    // CLOSE_WAIT
    if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::SYN_ACKED)
        _linger_after_streams_finish = false;

    // 如果到了准备断开连接的时候。服务器端先断
    // CLOSED
    if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED && !_linger_after_streams_finish) {
        is_active = false;
        return;
    }

    pack_with_winsize_ack_send();           //对应前面的send_empty_segment

}

bool TCPConnection::active() const { 
    if((_sender.stream_in().error() || _receiver.stream_out().error()))return false;
    // if(!_receiver.stream_out().eof() || !_sender.stream_in().eof()){
    //     return true;
    // }
    // if(_sender.get_fin_sent() && _sender.get_outstanding_list().empty()){
    //     if(_linger_after_streams_finish){
    //         return time_since_last_segment_received() < _cfg.rt_timeout * 10;
    //     }else{
    //         return false;
    //     }
    // }
    return is_active;
}

//Write data to the outbound byte stream, and send it over TCP if possible
size_t TCPConnection::write(const string &data) {
    size_t write_num = _sender.stream_in().write(data);
    _sender.fill_window();
    pack_with_winsize_ack_send();
    return write_num;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 

    assert(_sender.segments_out().empty());
    time_pass += ms_since_last_tick;

    //tell the tcpsender about the passage of time
    _sender.tick(ms_since_last_tick);

    //try to abort the connection
   
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS){
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        //TODO: 是否设置
        // _linger_after_streams_finish = false;

        TCPHeader tcpheader;
        tcpheader.seqno = _sender.next_seqno(); // seqo
        tcpheader.rst = 1;
        string readStr = "";
        Buffer payload(std::move(readStr));
        TCPSegment segment(tcpheader, payload);
        _segments_out.push(segment);
        return;
    }else{
        // 判断 TCP 断开连接时是否时需要等待
        // TIME_WAIT
        if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV &&
            TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED && _linger_after_streams_finish && time_since_last_segment_received() >= _cfg.rt_timeout * 10){
            is_active = false;
            return;
        }
            

        
    }

    //超时重传, _sender的tick中确实把需要重传的seg放入了sender的queue中，但是并没有放入tcpconnection的queue中
    pack_with_winsize_ack_send();
    
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    //TODO: 在输入流结束时必须立即发送fin
    _sender.fill_window();
    
    pack_with_winsize_ack_send();


}

void TCPConnection::connect() {
    //Initiate a connection by sending a SYN segment
 
 /**以下这种方式没有追踪系统的seqno变化
  * TCPHeader tcpheader;
    tcpheader.seqno = _sender.next_seqno(); // seqo
    tcpheader.syn =  1;
    string readStr = "";
    Buffer payload(std::move(readStr));
    TCPSegment segment(tcpheader, payload);
    _segments_out.push(segment);
    std::list<TCPSegment>& outlist = _sender.get_outstanding_list();
    outlist.push_back(segment);
  * 
  * 
 */
    _sender.fill_window();
    pack_with_winsize_ack_send();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            _sender.stream_in().set_error();
            _receiver.stream_out().set_error();
            TCPHeader tcpheader;
            tcpheader.seqno = _sender.next_seqno(); // seqo
            tcpheader.rst = 1;
            string readStr = "";
            Buffer payload(std::move(readStr));
            TCPSegment segment(tcpheader, payload);
            _segments_out.push(segment);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}


void TCPConnection::pack_with_winsize_ack_send(){
    
    //将sender中待发送的数据加上本地的win_size以及ackno，ackflag后，由TCPConnection进行发送
    std::queue<TCPSegment>& queue = _sender.segments_out();
    int size = queue.size();
    for(int i = 0; i < size; i++){
        TCPSegment& segfront = queue.front();
        //TODO: 类型不一致问题
        uint16_t maxValue = std::numeric_limits<uint16_t>::max();
        segfront.header().win = _receiver.window_size() > maxValue ? maxValue : _receiver.window_size();
        if(_receiver.ackno().has_value()){
            segfront.header().ack = 1;
            segfront.header().ackno = _receiver.ackno().value();
        }
        _segments_out.push(segfront);
        queue.pop();
    }
}