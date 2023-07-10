#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

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
    , con_retran_times(0)
    , win_size(1)
    , bytes_flight(0){}

uint64_t TCPSender::bytes_in_flight() const { return {}; }

void TCPSender::fill_window() {
    
    // if(win_size == 0){
    //     win_size = 1;
    // }

    // //window_size由receiver传回，表示的是reassembler中未写入ByteStream的部分
    // uint16_t real_win_size = win_size - bytes_in_flight();

    // //接收窗口的大小可能大于一个TCPSegment的最大大小,这样就需要构造多个segment
    // while (real_win_size != 0)
    // {
    //     if(real_win_size >= TCPConfig::MAX_PAYLOAD_SIZE){
    //         //构造一个最大的TCPSegment
    //         TCPHeader tcpheader;
    //         tcpheader.seqno = next_seqno();
    //         tcpheader.syn =  next_seqno_absolute() == 0;
    //         string readStr = _stream.peek_output(TCPConfig::MAX_PAYLOAD_SIZE);
    //         _stream.pop_output(TCPConfig::MAX_PAYLOAD_SIZE);
    //         //如何判断这是否是一个fin报文
    //         tcpheader.fin = _stream.buffer_empty();
    //         Buffer payload(std::move(readStr));
    //         TCPSegment segment(tcpheader, payload);
    //         _segments_out.push(segment);
    //         outstanding_list.push_back(segment);
    //         real_win_size -= segment.length_in_sequence_space();
    //         bytes_flight += segment.length_in_sequence_space();
    //         _next_seqno += segment.length_in_sequence_space();
    //     }else{
    //         //构造一个real_win_size大小的TCPSegment
    //         TCPHeader tcpheader;
    //         tcpheader.seqno = next_seqno();
    //         tcpheader.syn =  next_seqno_absolute() == 0;
    //         string readStr = _stream.peek_output(real_win_size);
    //         _stream.pop_output(real_win_size);
    //         //如何判断这是否是一个fin报文
    //         tcpheader.fin = _stream.buffer_empty();
    //         Buffer payload(std::move(readStr));
    //         TCPSegment segment(tcpheader, payload);
    //         _segments_out.push(segment);
    //         outstanding_list.push_back(segment);
    //         real_win_size = 0;
    //         bytes_flight += segment.length_in_sequence_space();
    //         _next_seqno += segment.length_in_sequence_space();
    //     }
    // }
    
    
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    win_size = window_size; 
    uint64_t abso_seqno = unwrap(ackno, _isn,1);
    abso_seqno++;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }

unsigned int TCPSender::consecutive_retransmissions() const { return con_retran_times; }

void TCPSender::send_empty_segment() {}
