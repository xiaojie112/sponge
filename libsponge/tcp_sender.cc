#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>
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
    , bytes_flight(0)
    , timer_start(0)
    , timer_on(false)
    , time_pass(0)
    , retransmission_timeout(retx_timeout)
    , fin_sent(false){}

uint64_t TCPSender::bytes_in_flight() const { return bytes_flight; }

void TCPSender::fill_window() {
    
    // if(win_size == 0){
    //     // win_size = 1;  这里不能真的设置为1
    // }

    // cout << "bytes_in_flight: " << bytes_flight << endl;
    // cout << "winsize: " << win_size << endl;
    // cout << "real_winsize: " << win_size - bytes_in_flight() << endl;
    //window_size由receiver传回，表示的是reassembler中未写入ByteStream的部分
    if((win_size == 0 && 1 <= bytes_flight) || (win_size != 0 && win_size <= bytes_flight))return;
    uint16_t real_win_size = win_size == 0 ? 1 - bytes_in_flight() : win_size - bytes_in_flight();

    //接收窗口的大小可能大于一个TCPSegment的最大大小,这样就需要构造多个segment
    while (real_win_size != 0)
    {
        if(real_win_size >= TCPConfig::MAX_PAYLOAD_SIZE){
            //如何处理对fin报文的ack， fin报文已经ack了， 那么即使有空间发送数据， 也不能再发送segment
            if(_stream.eof() && _stream.bytes_written()+2 == next_seqno_absolute() && bytes_in_flight() == 0)return;

            //如何处理stream中没有数据了，eof的情况, 如何避免反复发送无数据的fin报文
            if(fin_sent)return;

            //构造一个最大的TCPSegment
            TCPHeader tcpheader;
            tcpheader.seqno = next_seqno();
            tcpheader.syn =  next_seqno_absolute() == 0;
            string readStr = _stream.read(TCPConfig::MAX_PAYLOAD_SIZE);
            //如何判断这是否是一个fin报文
            // tcpheader.fin = _stream.buffer_empty();
            tcpheader.fin = _stream.eof() && readStr.length() < real_win_size;  //"Don't add FIN if this would make the segment exceed the receiver's window"
            fin_sent = tcpheader.fin;
            Buffer payload(std::move(readStr));
            TCPSegment segment(tcpheader, payload);

            //segment如果占用的seq空间为0则不用创建和发送segment，什么时候会出现这种情况？数据长度为0，且没有设置syn和fin标志
            if(segment.length_in_sequence_space() == 0)return;
            _segments_out.push(segment);
            outstanding_list.push_back(segment);
            real_win_size -= segment.length_in_sequence_space();
            bytes_flight += segment.length_in_sequence_space();
            // cout <<"check1: " << bytes_flight << endl;

            _next_seqno += segment.length_in_sequence_space();

            if(timer_on)timer_start = time_pass;
        }else{
            //TODO: 如何处理对fin报文的ack， fin报文已经ack了， 那么即使有空间发送数据， 也不能再发送segment
            if(_stream.eof() && _stream.bytes_written()+2 == next_seqno_absolute() && bytes_in_flight() == 0)return;

            //TODO: 如何处理stream中没有数据了，eof的情况, 如何避免反复发送无数据的fin报文
            if(fin_sent)return;

            //构造一个real_win_size大小的TCPSegment
            TCPHeader tcpheader;
            tcpheader.seqno = next_seqno(); // seqo
            tcpheader.syn =  next_seqno_absolute() == 0;
            // cout << "check1: " << _stream.buffer_empty() << endl;
            string readStr = _stream.read(real_win_size);
            // cout << "check2: " << readStr << endl;
            //如何判断这是否是一个fin报文
            // tcpheader.fin = _stream.buffer_empty();
            //TODO: stream的eof的标志是如何设置的
            tcpheader.fin = _stream.eof() && readStr.length() < real_win_size;  //"Don't add FIN if this would make the segment exceed the receiver's window"
            fin_sent = tcpheader.fin;
            Buffer payload(std::move(readStr));
            //TODO: 如果stream中暂时已经没有数据了，但是也没有eof, 但是这里仍然会创造segment,如何解决
            TCPSegment segment(tcpheader, payload);
            if(segment.length_in_sequence_space() == 0)return;
            _segments_out.push(segment);
            outstanding_list.push_back(segment);
            real_win_size = 0;
            bytes_flight += segment.length_in_sequence_space();
            // cout <<"check1: " << bytes_flight << endl;

            // cout << "check1: "<< _next_seqno << endl;
            
            _next_seqno += segment.length_in_sequence_space(); // abseqo

            // cout << "check2: " << _next_seqno << endl;

            if(!timer_on){
                timer_start = time_pass;
                timer_on = true;
            }
        }
    }
    
    // cout << "next_seqno_absolute: " << next_seqno_absolute() << endl;
    // cout << bytes_in_flight();
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    win_size = window_size; 


    //比较ackno和segment.seqo有问题
    //TODO: 是将seqo转换成absoseq还是将absseq转换成seqo
    uint64_t abso_seqno = unwrap(ackno, _isn, _stream.bytes_read());

    //TODO: 错误的ackno处理
    if(abso_seqno > _next_seqno)return;
    
    //遍历outstanding segment集合
    auto seg = outstanding_list.begin();
    while(seg != outstanding_list.end()){
        // cout << "check1:" <<unwrap((seg->header()).seqno, _isn, _stream.bytes_read()) << endl;
        // cout << "check2:" << seg->length_in_sequence_space() << endl;
        //TODO: checkpoint的选取
        if(unwrap((seg->header()).seqno, _isn, _stream.bytes_read()) + seg->length_in_sequence_space() <= abso_seqno){
            timer_start = time_pass;       //更新超时重传计时器
            bytes_flight -= seg->length_in_sequence_space();
            // cout << "check2: " << bytes_flight << endl;

            seg = outstanding_list.erase(seg);
            con_retran_times = 0;
            retransmission_timeout = _initial_retransmission_timeout;
        }else{
            seg++;
        }
    }
    
    if(outstanding_list.empty())timer_on = false;//关闭超时重传计时器

    //如果腾出了新的空间，继续fill the window
    //TODO: 这一行代码去掉了也无妨 fill_window();

    // cout << "check1:" << _segments_out.empty() << endl;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    time_pass += ms_since_last_tick;
    if(timer_on && time_pass - timer_start >= retransmission_timeout){
        //超时重传最早的segment
        // outstanding_list.pop_front()  //不能弹出，不能确保ack
        TCPSegment seg = outstanding_list.front();
        _segments_out.push(seg);
        timer_start = time_pass;

        //TODO: window_size非0才会进行如下设置
        if(win_size != 0){
            con_retran_times++;  
            retransmission_timeout *= 2;
        }
        
    }

    //TODO: end up sending a segment
    // fill_window();
}

unsigned int TCPSender::consecutive_retransmissions() const { return con_retran_times; }

void TCPSender::send_empty_segment() {}
