#include "byte_stream.hh"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <iostream>
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : buffer(), cpty(capacity) {}

size_t ByteStream::write(const string &data) {
    //TODO: 考虑可能有bug
    // if(endInput == true)return 0;  这里考虑是否会有bug, 有可能已经endInput但是仍然有segment携带数据过来

    // DUMMY_CODE(data);
    //尽可能写多的字节并返回成功写入的字节数
    size_t wirteCount = 0;
    for (char c : data) {
        // cout << "check2:" << buffer.size() << endl;
        if (buffer.size() == getCapacity())
            break;
        getBuffer().push_front(c);
        wirteCount++;
    }
    
    writeNum += wirteCount;
    return wirteCount;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    // DUMMY_CODE(len);
    //这里考虑了想要查看的字节数超出了buffer的容量
    size_t length = len > buffer.size() ? buffer.size() : len;
    // cout <<"checkout2:" << buffer.size() << endl;
    // cout << "checkout1:" << (len > buffer.size() ? buffer.size() : len) << endl;
    // size_t length = len;
    list<char> lisToString;
    size_t temp = 0;
    while (length != temp) {
        // cout << "checkout3: " << buffer.at(length-1) << endl;
        lisToString.push_back(buffer.at(buffer.size() - temp-1));
        temp++;
    }
    string str(lisToString.begin(), lisToString.end());
    return str;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    // DUMMY_CODE(len);
    //如何考虑出错的情况
    size_t length = len > buffer.size() ? buffer.size() : len;
    readNum += length;
    // size_t length = len;
    while (length != 0) {
        buffer.pop_back();
        length--;
    }
    
}

void ByteStream::end_input() { endInput = true; }

bool ByteStream::input_ended() const { return endInput; }

size_t ByteStream::buffer_size() const { return buffer.size(); }

bool ByteStream::buffer_empty() const { return buffer.size() == 0; }

bool ByteStream::eof() const { return buffer_empty() && endInput; }

size_t ByteStream::bytes_written() const { return writeNum; }

size_t ByteStream::bytes_read() const { return readNum; }

size_t ByteStream::remaining_capacity() const { return cpty - buffer.size(); }
