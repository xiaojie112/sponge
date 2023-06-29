#include "tcp_receiver.hh"
#include <iostream>
// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    // Set the Initial Sequence Number if necessary
    if(seg.header().syn == false && setISN == false)return;//这个bug找了很久，还没有建立连接但是收到了非syn报文，直接丢弃
    if(seg.header().syn == true){
        isn = seg.header().seqno;
        setISN = true;
    }
    

    //Push any data, or end-of-stream marker, to the StreamReassembler.
    //. Keep in mind that SYN and FIN aren’t part of the stream itself and aren’t “bytes”—they represent the beginning and ending of the byte stream itself.
    //In your TCP implementation, you’ll use the index of the last reassembled byte as the checkpoint.

    //Note that the SYN flag is just one flag in the header. The same segment could also carry data and could even have the FIN flag set.


    bool eof = seg.header().fin;
    string data = seg.payload().copy();
    uint64_t checkpoint = stream_out().bytes_written() == 0? 0 : stream_out().bytes_written()-1;  // 这个checkpoint到底什么鬼？
    uint64_t index = unwrap(seg.header().seqno, isn,checkpoint);
    if(seg.header().syn){
        index = 0;
    }else{
        index -= 1;
    }
    // else if(seg.header().fin && data.length() == 0){ // fin报文且数据长度为0的情况如何考虑?  syn报文且数据长度为0的情况如何考虑?  
    //     index -= 2;
    // }
    _reassembler.push_substring(data, index, eof);
    
}

//报文段发送过来的时候，有一部分接收，一部分没有接收，如何返回ackno?
optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(!setISN){
        return {}; 
    }
    // cout << "-------------------"  << stream_out().bytes_written() << endl;
    // syn segment或者fin segment存在data.length == 0的情况, ByteStream在写入字节时对于data.length()==0不会写入任何字节, 但是syn和fin仍然占用seqno
    if(stream_out().bytes_written() == 0){      //true则此时发送过来的是syn报文且data.length==0, 或者此时发送过来的是syn和fin报文, data.length == 0
        if(stream_out().input_ended()){
            return isn + 2;   //注意syn和fin各占一个seqno
        }else{
            return isn + 1;
        }
        
    }

    //fin报文携带了数据的情况
    return stream_out().input_ended() ? wrap(stream_out().bytes_written()+1, isn) + 1 : wrap(stream_out().bytes_written()+1, isn); 
}

size_t TCPReceiver::window_size() const { 
    return stream_out().remaining_capacity(); 
}
