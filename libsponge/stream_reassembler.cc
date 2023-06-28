#include "stream_reassembler.hh"
#include <iostream>
#include <unordered_set>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : setEOF(0), unasm_bytes(0),reassemblerBuffer(), needIndex(0),
_output(capacity), _capacity(capacity){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    setEOF |= eof;
    //旧数据直接抛弃掉
    if(index + data.length() < needIndex + 1){
        //eof的情况不要忘记考虑
        if(eof && unasm_bytes == 0){
            _output.end_input();
        }
        return;
    }

    unordered_set<size_t> indexToDelete;
    
    string temp = data;
    //超出容量限制范围(bytestream + reassemblerBuffer <= capacity)之外的部分直接截取掉
    size_t maxIndex = needIndex + _capacity - _output.buffer_size() - 1;
    size_t curIndex = index;
    if(curIndex > maxIndex)return;
    if(curIndex + data.length() > 1 + maxIndex){
        temp = temp.substr(0,maxIndex-curIndex+1);
        setEOF = false;
    }

    //有部分与ByteStream已经写入的字节重叠
    if(curIndex < needIndex){
        temp = temp.substr(needIndex - curIndex);
        curIndex = needIndex;
    }

    // 遍历哈希表截取不重叠部分
    for (const auto& pair : reassemblerBuffer) {
        size_t curEndIndex = curIndex + temp.length() - 1;
        size_t startIndex = pair.first;
        // cout << "-----------------------" << reassemblerBuffer.at(startIndex) << endl;
        size_t endIndex = pair.first + pair.second.length() - 1;
        if(curIndex >= startIndex && curEndIndex <= endIndex){
            return;
        }
        if(curIndex <= startIndex && curEndIndex >= endIndex){
            indexToDelete.insert(startIndex);
        }
        if(curIndex > startIndex && curIndex <= endIndex){
            temp = temp.substr((endIndex-curIndex)+1);
            curIndex = endIndex+1;
        }
        if(curEndIndex < endIndex && curEndIndex >= startIndex){
            temp = temp.substr(0,temp.length()-(curEndIndex-startIndex)-1);
        }

    }
    
    //删除元素
    for(const auto& key : indexToDelete){
        unasm_bytes -= reassemblerBuffer[key].length();
        reassemblerBuffer.erase(key);
    }

    //成功截取不重叠的部分并放入哈希表中
    if(temp.length() != 0){
        reassemblerBuffer[curIndex] = temp;
        unasm_bytes += temp.length();
    }
    

    if(reassemblerBuffer.count(needIndex) > 0){
        //拼接
        string concat = "";
        size_t tempIndex = needIndex;
        string tempStr;
        while(reassemblerBuffer.count(tempIndex) > 0){
            tempStr = reassemblerBuffer[tempIndex];
            concat += tempStr;
            reassemblerBuffer.erase(tempIndex);
            tempIndex += tempStr.length();
            unasm_bytes -= tempStr.length();
        }
        needIndex += concat.length();
        //读入字节流
        _output.write(concat);
    }

    if(setEOF && unasm_bytes == 0){
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return unasm_bytes; }

bool StreamReassembler::empty() const { return unasm_bytes == 0; }
