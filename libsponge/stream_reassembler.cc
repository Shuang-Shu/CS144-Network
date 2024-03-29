#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity),
unassemBuf(), assemBuf(), expectIdx(0), unassemSize(0), eofIndex(-1) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if(eof)
        eofIndex=index+data.length();
    // 处理部分
    pushUnassem(data, index);
    fixCap();
    assem();
    writeStr();
    // 终止输入
    if(assemBuf.length()==0&&expectIdx==eofIndex)
        _output.end_input();
}

void StreamReassembler::pushUnassem(string data, size_t index){
    // 预处理
    string validStr;
    size_t validIdx,rightBound;
    rightBound=index+data.length();
    if(index+data.length()<=expectIdx)
        return;
    else if(index<expectIdx){
        validIdx=expectIdx;
        validStr=data.substr(expectIdx-index, data.length()-(expectIdx-index));
    }
    else{
        validIdx=index;
        validStr=data;
    }
    // 将validStr插入到unassemBuf中
    if(unassemBuf.find(validIdx)==unassemBuf.end())
        unassemBuf.insert(make_pair(validIdx, validStr));
    else{
        if((*unassemBuf.find(validIdx)).second.length()>=validStr.length())
            return;
        else
            (*unassemBuf.find(validIdx)).second=validStr;
    }
    auto iterR=unassemBuf.find(validIdx);
    auto iterRL=iterR;
    auto iterL=iterR;
    iterL--;
    if(iterL==unassemBuf.end())
        iterL=iterR;
    else if((*iterL).first+(*iterL).second.length()<=(*iterR).first)
        iterL=iterR;
    rightBound=max((*iterL).first+(*iterL).second.length(), rightBound);
    while(iterR!=unassemBuf.end()&&(*iterR).first<validIdx+validStr.length()){
        rightBound=max(rightBound, (*iterR).first+(*iterR).second.length());
        iterRL=iterR;
        iterR++;
    }
    string mergedStr(rightBound-(*iterL).first, '-');
    size_t leftBound=(*iterL).first;
    for(auto iter=iterL;iter!=iterR;++iter){
        size_t offset=(*iter).first;
        string tempStr=(*iter).second;
        for(size_t j=0;j<tempStr.length();++j)
            mergedStr[j+offset-leftBound]=tempStr[j];
    }
    (*iterL).second=mergedStr;
    if(iterL!=iterR){
        iterL++;
        vector<size_t> deleteIdx;
        for(auto iter=iterL;iter!=iterR;++iter)
            deleteIdx.push_back((*iter).first);
        for(size_t i=0;i<deleteIdx.size();++i)
            unassemBuf.erase(deleteIdx[i]);
    }
}

void StreamReassembler::assem(){
    auto iter=unassemBuf.begin();
    while(iter!=unassemBuf.end()&&(*iter).first==expectIdx){
        assemBuf+=(*iter).second;
        size_t temp=(*iter).first;
        expectIdx+=(*iter).second.length();
        iter++;
        unassemBuf.erase(temp);
    }
    size_t newSize=0;
    for(iter=unassemBuf.begin();iter!=unassemBuf.end();++iter)
        newSize+=(*iter).second.length();
    unassemSize=newSize;
}

void StreamReassembler::writeStr(){
    size_t writeLen=_output.write(assemBuf);
    assemBuf=assemBuf.substr(writeLen, assemBuf.length()-writeLen);
}

void StreamReassembler::fixCap(){
    if(unassemBuf.size()==0)
        return;
    size_t temp=0;
    for(auto iter=unassemBuf.begin();iter!=unassemBuf.end();++iter)
        temp+=(*iter).second.length();
    if(_capacity>assemBuf.length()+temp)
        return;
    else{
        auto iter=unassemBuf.end();iter--;
        size_t overflowSize=assemBuf.length()+temp-_capacity;
        while((*iter).second.length()<=overflowSize){
            overflowSize-=(*iter).second.length();
            size_t tempIdx=(*iter).first;
            --iter;
            unassemBuf.erase(tempIdx);
        }
        size_t tempLen=(*iter).second.length();
        (*iter).second=(*iter).second.substr(0, tempLen-overflowSize);
    }
}

size_t StreamReassembler::unassembled_bytes() const { return unassemSize; }

bool StreamReassembler::empty() const {
    if(unassemSize+assemBuf.length()==0)
        return true;
    return false;
}