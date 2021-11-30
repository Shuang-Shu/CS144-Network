#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity0) : capacity(capacity0), _output(capacity0), unassembledBuf(), idx_heap(), assembledBuf(), unassembledSize(0), expectedIdx(0), eof(false){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool aEof) {
    // DUMMY_CODE(data, index, eof);
    //cout<<"=========================================="<<endl;
    this->eof=this->eof|aEof;
    // 进行输入之前，先检查data的index是否满足index<=this.expectedIdx
    if(index<=this->expectedIdx)
        this->pushData(data, index);
    this->insertStr(data, index);
    this->assembleStr();
    this->writeStr();
    //cout<<"=========================================="<<endl;
}

void StreamReassembler::pushData(const string &data, const size_t &index){
    if(index+data.length()<=this->expectedIdx)
        // 若该data并无有用信息
        return;
    if(this->assembledBuf.length()!=0)
        // 此时有序的内容还存在，只是未发送，直接丢弃新的内容
        return;
    size_t validLength=index+data.length()-this->expectedIdx;
    string validStr=data.substr(data.length()-validLength, validLength);
    size_t writeLength=this->_output.write(validStr);
    this->expectedIdx+=writeLength;
}

map<size_t, string>::iterator StreamReassembler::detectOverlap(size_t index, size_t length, size_t &leftOverlap, size_t &rightOverlap, size_t &mergeLength){
    this->unassembledBuf.insert(make_pair(index, ""));
    map<size_t, string>::iterator iterLeft=this->unassembledBuf.find(index);

    map<size_t, string>::iterator end=this->unassembledBuf.end();
    map<size_t, string>::iterator begin=this->unassembledBuf.begin();
    begin--;
    iterLeft--;
    map<size_t, string>::iterator iterRight=this->unassembledBuf.find(index);
    iterRight++;
    size_t rightBound=index+length;
    while(iterLeft!=begin&&(*iterLeft).first+(*iterLeft).second.length()>=index){
        leftOverlap++;
        iterLeft--;
    }
    while(iterRight!=end&&(*iterRight).first<rightBound){
        rightOverlap++;
        iterRight++;
    }
    iterLeft++;
    iterRight--;
    mergeLength=(*(iterRight)).first+(*(iterRight)).second.length()-(*(iterLeft)).first;
    return iterLeft;
}

void StreamReassembler::insertStr(const string &data, size_t index){
    // 首先检查buf中是否已经有该index
    // 存在的问题：无法检查重叠的substr
    size_t leftOverlap=0;
    size_t rightOverlap=0;
    size_t mergeLength=0;

    auto iter=this->detectOverlap(index, data.length(), leftOverlap, rightOverlap, mergeLength);
    auto tempIter=iter;

    size_t startIndex=(*tempIter).first;
    size_t totalNum=leftOverlap+rightOverlap+1;
    size_t oldLength=0;
    vector<size_t> overlapIndex;

    string mergeStr(mergeLength, '-');
    for(size_t i=0;i<totalNum;++i){
        oldLength+=(*tempIter).second.length();
        string tempStr=(*tempIter).second;
        for(size_t j=0;j<tempStr.length();++j){
            mergeStr[(*tempIter).first+j-startIndex]=tempStr[j];
        }
        overlapIndex.push_back((*tempIter).first);
        tempIter++;
    }
    this->unassembledBuf[startIndex]=mergeStr;
    for(size_t i=1;i<totalNum;++i)
        this->unassembledBuf.erase(overlapIndex[i]);
    this->unassembledSize-=(oldLength-mergeStr.length());

    /*
    // cout<<"data; "<<data.substr(0, 10)<<";index: "<<index<<endl;
    if(this->unassembledBuf.find(index)!=this->unassembledBuf.end()){
        size_t oldLength=this->unassembledBuf[index].length();
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
    */
}

void StreamReassembler::assembleStr(){
    //cout<<"期望的index: "<<this->expectedIdx<<"目前的最小索引: "<<this->peek()<<endl;
    while(!(this->idx_heap.empty())&&this->expectedIdx>=this->peek()){
        size_t peekVal=this->pop();
        string &strRef=this->unassembledBuf[peekVal];
        //cout<<"peekVal: "<<peekVal<<";assembledStr: "<<this->assembledBuf<<";strRef: "<<strRef<<endl;
        this->unassembledBuf.erase(peekVal);
        this->unassembledSize-=strRef.length();
        if(peekVal+strRef.length()<=this->expectedIdx){
            continue;
        }else{
            size_t validLength=strRef.length()-(this->expectedIdx-peekVal);
            this->assembledBuf+=strRef.substr(this->expectedIdx-peekVal, validLength);
            this->expectedIdx+=validLength;
        }
    }
}

void StreamReassembler::writeStr(){
    //cout<<"aStr: "<<this->assembledBuf<<endl;
    size_t writeLength=this->_output.write(this->assembledBuf);
    this->assembledBuf=this->assembledBuf.substr(writeLength, this->assembledBuf.length()-writeLength);
    if(this->eof&&this->assembledBuf.length()==0&&this->unassembledSize==0)
        this->_output.end_input();
}

void StreamReassembler::insertToHeap(size_t val){
    vector<size_t> &heapRef=this->idx_heap;
    heapRef.push_back(val);
    make_heap(heapRef.begin(), heapRef.end(), greater<int>());
}

size_t StreamReassembler::peek(){
    return this->idx_heap[0];
}

size_t StreamReassembler::pop(){
    size_t peekVal=this->peek();
    pop_heap(this->idx_heap.begin(), this->idx_heap.end(), greater<int>());
    this->idx_heap.pop_back();
    return peekVal;
}

bool StreamReassembler::isFull(){
    if(this->unassembledSize+this->assembledBuf.length()<this->capacity)
        return true;
    return false;
}

size_t StreamReassembler::unassembled_bytes() const { return this->unassembledSize; }

bool StreamReassembler::empty() const { return {}; }
