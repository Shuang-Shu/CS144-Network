#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), capacity(capacity)
, unassembledBuf(), idx_heap(), assembledBuf(), unassembledSize(), expectedIdx(0), eof(false){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // DUMMY_CODE(data, index, eof);
    this->eof=this->eof|eof;
    this->insertStr(data, index);
    this->assembleStr();
    this->writeStr();
}

void StreamReassembler::insertStr(const string &data, size_t index){
    // 首先检查buf中是否已经有该index
    if(this->unassembledBuf.find(index)!=this->unassembledBuf.end()){
        size_t oldLength=this->unassembledBuf[index].length()
        if(data.length()<=oldLength)
            return;
        else{
            // 要检查capacity是否足够
            size_t leftSpace=this->capacity-this->unassembledSize-this->assembledBuf.length();
            size_t validLength=min(leftSpace, data.length()-oldLength);
            this->unassembledSize+=validLength;
            this->unassembledBuf[index]=data.substr(0, validLength+oldLength);
        }
    }else{
        this->insertToHeap(index);
        size_t leftSpace=this->capacity-this->unassembledSize-this->assembledBuf.length();
        size_t validLength=min(leftSpace, data.length());
        this->unassembledSize+=validLength;
        this->unassembledBuf.insert(make_pair(index, data.substr(0, validLength)));
    }
}

void StreamReassembler::assembleStr(){
    while(!(this->idx_heap.empty())&&this->expectedIdx>=this->idx_heap[0]){
        size_t peekVal=this->pop();
        string &strRef=this->unassembledBuf[peekVal];
        this->unassembledSize-=strRef.length();
        if(peekVal+strRef.length()<=this->expectedIdx){
            this->unassembledBuf.erase(peekVal);
            continue;
        }else{
            size_t validLength=strRef.length()-(this->expectedIdx-peekVal);
            this->assembledBuf+=strRef.substr(this->expectedIdx-peekVal, validLength);
            this->expectedIdx+=validLength;
        }
    }
}

void StreamReassembler::writeStr(){
    size_t writeLength=this->_output.write(this->assembledBuf);
    this->assembledBuf=this->assembledBuf.substr(writeLength, this->assembledBuf.length()-writeLength);
    if(this->eof&&this->assembledBuf.length()==0&&this->unassembledSize==0)
        this->_output.end_input();
}

void StreamReassembler::insertToHeap(size_t){
    vector<size_t> &heapRef=this->idx_heap;
    heapRef.insert(size_t);
    push_heap(heapRef.begin(), heapRef.end());
}

size_t StreamReassembler::peek(){
    return this->idx_heap[0];
}

size_t StreamReassembler::pop(){
    size_t peekVal=this->peek();
    pop_heap(this->idx_heap);
    this->idx_heap.pop_back();
    return peekVal;
}

size_t StreamReassembler::unassembled_bytes() const { return this->unassembledSize; }

bool StreamReassembler::empty() const { return {}; }
