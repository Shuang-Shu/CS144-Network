#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity0) : 
capacity(capacity0), _output(capacity0), unassembledBuf(), 
idx_heap(), assembledBuf(), unassembledSize(0), expectedIdx(0), eof(false){}

bool StreamReassembler::isFull(){
    if(this->unassembledSize+this->assembledBuf.length()==this->capacity)
        return true;
    return false;
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool aEof) {
    // DUMMY_CODE(data, index, eof);
    //cout<<"=========================================="<<endl;
    this->eof=this->eof|aEof;
    // 进行输入之前，先检查data的index是否满足index<=this.expectedIdx
    if(this->isFull()&&index<=this->expectedIdx)
        this->pushData(data, index);
    this->insertStr(data, index);
    this->assembleStr();
    cout<<"未组装大小: "<<this->unassembledSize<<endl;
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

// detectOverlap：检查与新data重叠的str
// index:data的首字节对应的index
// data:传入的substring
// leftOverlap:index左侧的与data重叠的substring数量
// rightOverlap:index右侧的与data重叠的substring数量
// mergedLength:data与旧的重叠的substring组合的新融合string长度
// oldLength:所有unassembledBuf中与data重叠的substring总长度

map<size_t, string>::iterator StreamReassembler::detectOverlap(size_t index, const string &data, size_t &leftOverlap, 
size_t &rightOverlap, size_t &mergedLength, size_t &oldLength){
    // 首先检查该index是否在map中
    auto findIter=this->unassembledBuf.find(index);
    if(findIter!=this->unassembledBuf.end()){
        if((*findIter).second.length()>data.length())
            // 没有新的信息，不处理
            return findIter;
        else
            // 因为此时data的长度大于buf中同index的内容，则旧内容一定被覆盖
            this->unassembledBuf.erase(index);
    }
    this->unassembledBuf.insert(make_pair(index, data));
    size_t rightBound=index+data.length();

    auto iter=this->unassembledBuf.find(index);
    map<size_t, string>::reverse_iterator riter(this->unassembledBuf.find(index));

    auto end=this->unassembledBuf.end();
    auto rend=this->unassembledBuf.rend();

    iter++;
    riter++;

    while(iter!=end&&(*iter).first<=rightBound){
        oldLength+=(*iter).second.length();
        rightOverlap++;
        iter++;
    }
    iter--;
    while(riter!=rend&&((*riter).first+(*riter).second.length()>=index)){
        oldLength+=(*riter).second.length();
        leftOverlap++;
        riter++;
    }
    riter--;
    mergedLength=rightBound-(*riter).first;
    return --(riter.base());

    /*
    this->unassembledBuf.insert(make_pair(index, data));
    auto iterLeft=map<size_t, string>::reverse_iterator(this->unassembledBuf.find(index));
    iterLeft++;

    auto end=this->unassembledBuf.end();
    auto rend=this->unassembledBuf.rend();
    
    auto iterRight=this->unassembledBuf.find(index);
    iterRight++;

    size_t rightBound=index+data.length();
    while(iterLeft!=rend&&(*iterLeft).first+(*iterLeft).second.length()>=index){
        leftOverlap++;
        iterLeft++;
    }
    while(iterRight!=end&&(*iterRight).first<rightBound){
        rightOverlap++;
        iterRight++;
    }

    //cout<<(*iterLeft).first<<endl;
    iterLeft--;
    iterRight--;
    //cout<<(*iterLeft).first<<endl;
    //cout<<index<<", "<<this->unassembledBuf.size()<<", "<<((*(iterRight)).first)<<", "<<(*(iterRight)).second.length()<<", "<<(*(iterLeft)).first<<endl;
    mergeLength=(*(iterRight)).first+(*(iterRight)).second.length()-(*(iterLeft)).first;
    //cout<<mergeLength<<endl;
    //cout<<leftOverlap<<", "<<rightOverlap<<endl;
    cout<<(*iterLeft).first<<", "<<(*(--iterLeft.base())).first<<endl;
    return --(iterLeft.base());
    */
}

// merge函数，根据传入的iter和overlapNum，自动将unassembledBuf中的重叠substring组合
// iter：开始重叠的第一个substring对应的迭代器
// overlapNum：重叠的substring数量
// mergedLength：合并后的string长度

string StreamReassembler::merge(map<size_t, string>::iterator iter, size_t overlapNum, size_t mergedLength){
    vector<size_t> overlapIndex;
    auto tempIter=iter;
    tempIter++;
    for(size_t i=1;i<overlapNum;++i){
        overlapIndex.push_back((*tempIter).first);
    }
    string mergedStr(mergedLength, '-');
    size_t start=(*iter).first;
    size_t offset=0;
    for(size_t i=0;i<overlapNum;++i){
        offset=(*iter).first-start;
        string tempStr=(*iter).second;
        for(size_t j=0;j<tempStr.length();++j){
            mergedStr[offset+j]=tempStr[j];
        }
    }
    for(size_t i=0;i<overlapIndex.size();++i){
        this->unassembledBuf.erase(overlapIndex[i]);
    }
    return mergedStr;
}

void StreamReassembler::insertStr(const string &data, size_t index){
    size_t leftOverlap=0;
    size_t rightOverlap=0;
    size_t mergedLength=0;
    size_t oldLength=0;

    auto leftIter=this->detectOverlap(index, data, leftOverlap, rightOverlap, mergedLength, oldLength);
    string mergedStr=this->merge(leftIter, rightOverlap+leftOverlap+1, mergedLength);
    this->unassembledSize+=(mergedLength-oldLength);
    (*leftIter).second=mergedStr;

    /*
    auto tempIter=this->detectOverlap(index, data, leftOverlap, rightOverlap, mergeLength, oldLength);
    // auto iter=this->assembledBuf.find(index);
    //auto tempIter=iter;
    //cout<<(*tempIter).first<<endl;

    size_t startIndex=(*tempIter).first;
    //showMap(this->unassembledBuf);
    size_t totalNum=leftOverlap+rightOverlap+1;
    cout<<totalNum<<", "<<mergeLength<<", "<<(*tempIter).first<<endl;
    vector<size_t> overlapIndex;

    string mergeStr(mergeLength, '-');
    for(size_t i=0;i<totalNum;++i){
        string tempStr=(*tempIter).second;
        for(size_t j=0;j<tempStr.length();++j){
            mergeStr[(*tempIter).first+j-startIndex]=tempStr[j];
        }
        overlapIndex.push_back((*tempIter).first);
        tempIter++;
    }
    //this->unassembledBuf[startIndex]=mergeStr;
    for(size_t i=1;i<totalNum;++i)
        this->unassembledBuf.erase(overlapIndex[i]);
    this->unassembledSize+=(mergeStr.length()-oldLength);
    this->unassembledSize+=data.length();
    this->unassembledBuf.erase(startIndex);
    this->unassembledBuf.insert(make_pair(startIndex, mergeStr));
    this->insertToHeap(startIndex);
    cout<<this->unassembledBuf.size()<<endl;
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

size_t StreamReassembler::unassembled_bytes() const { return this->unassembledSize; }

bool StreamReassembler::empty() const { return {}; }

// 一些debug函数
void showMap(map<size_t, string> map0){
    auto iter=map0.begin();
    while(iter!=map0.end()){
        cout<<(*iter).second<<", ";
        iter++;
    }
    cout<<endl;
}
