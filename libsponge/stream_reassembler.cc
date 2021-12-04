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
// 已检查
void StreamReassembler::push_substring(const string &data, const size_t index, const bool aEof) {
    this->eof=this->eof|aEof;
    if(this->isFull()&&index<=this->expectedIdx)
        this->pushData(data, index);
    this->insertStr(data, index);
    this->refreshSize();
    this->assembleStr();
    this->writeStr();
}

// 已检查
void StreamReassembler::pushData(const string &data, const size_t &index){
    if(index+data.length()<=this->expectedIdx)
        return;
    if(this->assembledBuf.length()!=0)
        return;
    size_t validLength=index+data.length()-this->expectedIdx;
    string validStr=data.substr(data.length()-validLength, validLength);
    size_t writeLength=this->_output.write(validStr);
    this->expectedIdx+=writeLength;
    this->writeStr();
}

// detectOverlap：检查与新data重叠的str
// index:data的首字节对应的index
// data:传入的substring
// leftOverlap:index左侧的与data重叠的substring数量
// rightOverlap:index右侧的与data重叠的substring数量
// mergedLength:data与旧的重叠的substring组合的新融合string长度
// oldLength:所有unassembledBuf中与data重叠的substring总长度

// 已检查
map<size_t, string>::iterator StreamReassembler::detectOverlap(size_t index, const string &data, size_t &leftOverlap, 
size_t &rightOverlap, size_t &mergedLength, size_t &oldLength){
    // 首先检查该index是否在map中
    auto findIter=this->unassembledBuf.find(index);
    if(findIter!=this->unassembledBuf.end()){
        if((*findIter).second.length()>data.length())
            return findIter;
        else{
            oldLength+=(*findIter).second.length();
            this->unassembledBuf.erase(index);
        }
    }
    this->unassembledBuf.insert(make_pair(index, ""));
    size_t dataRightBound=index+data.length();
    size_t rightBound=dataRightBound;

    auto iter=this->unassembledBuf.find(index);
    map<size_t, string>::reverse_iterator riter(this->unassembledBuf.find(index));
    riter--;

    auto end=this->unassembledBuf.end();
    auto rend=this->unassembledBuf.rend();

    iter++;
    riter++;

    while(iter!=end&&(*iter).first<=dataRightBound){
        rightBound=max(rightBound, (*iter).first+(*iter).second.length());
        oldLength+=(*iter).second.length();
        rightOverlap++;
        iter++;
    }
    iter--;
    rightBound=dataRightBound>rightBound?dataRightBound:rightBound;
    while(riter!=rend&&((*riter).first+(*riter).second.length()>=index)){
        rightBound=max(rightBound, (*riter).first+(*riter).second.length());
        oldLength+=(*riter).second.length();
        leftOverlap++;
        riter++;
    }
    riter--;
    mergedLength=rightBound-(*riter).first;
    return --(riter.base());
}

// merge函数，根据传入的iter和overlapNum，自动将unassembledBuf中的满足空间大小的重叠substring组合，这其中不包括将要插入的数据data
// iter：开始重叠的第一个substring对应的迭代器
// overlapNum：重叠的substring数量，这里=left+right+1
// mergedLength：合并后的string长度


// 待更新，模式见goodnotes
// leftBound为合并串的左边缘
void StreamReassembler::merge(size_t index, const string &data, 
map<size_t, string>::iterator iter, size_t leftOverlap, 
size_t rightOverlap, size_t mergedLength){
    size_t leftSpace=this->capacity-this->unassembledSize-this->assembledBuf.length();
    size_t overlapNum=leftOverlap+rightOverlap;
    size_t leftBound=(*iter).first;
    size_t initLeftBound=leftBound;
    if(leftSpace!=0)
        this->insertToHeap(leftBound);
    if(leftOverlap+rightOverlap==0){
        // 若无重叠的内容（已完成）
        this->unassembledBuf.erase(index);
        this->unassembledBuf.insert(make_pair(leftBound, data.substr(0, min(data.length(), leftSpace))));
        return;
    }
    size_t gapSum=0;
    if(leftOverlap==0)
        // 若左侧无重叠
        iter++;
    auto startIter=iter;
    this->unassembledBuf.erase(index);
    size_t count=0;
    while(count<overlapNum){
        size_t newGap=((*iter).first-leftBound);
        gapSum+=newGap;
        leftBound=(*iter).first+(*iter).second.length();
        ++iter;
        if(gapSum>leftSpace){
            // 若gapSum>剩余的空间，证明此iter前的空间不能全部使用，需要截断data
            gapSum-=newGap;
            if(count==0){
                this->unassembledBuf.insert(make_pair(initLeftBound, data.substr(0, leftSpace)));
            }else{
                --iter;
                size_t rightBound=(*iter).first+(*iter).second.length()+(leftSpace-gapSum);
                string validStr(rightBound-initLeftBound, '-');
                auto temp=startIter;
                for(size_t i=0;i<count;++i){
                    string tempStr=(*temp).second;
                    for(size_t j=0;j<tempStr.length();++j)
                        validStr[j+(*temp).first-initLeftBound]=tempStr[j];
                }
                for(size_t i=index;i<rightBound;++i)
                    validStr[i-initLeftBound]=data[i-index];
                this->deleteOverlap(startIter, count);
                this->unassembledBuf.insert(make_pair(initLeftBound, validStr));
                return;
            }
        }
        ++count;
    }
    --iter;
    size_t rightBound=min((*iter).first+(*iter).second.length()+leftSpace-gapSum, initLeftBound+mergedLength);
    string validStr(rightBound-initLeftBound, '-');
    auto temp=startIter;
    for(size_t i=0;i<count;++i){
        string tempStr=(*temp).second;
        for(size_t j=0;j<tempStr.length();++j)
            validStr[j+(*temp).first-initLeftBound]=tempStr[j];
        ++temp;
    }
    for(size_t i=index;i<index+data.length();++i)
        validStr[i-initLeftBound]=data[i-index];
    this->deleteOverlap(startIter, count);
    this->unassembledBuf.insert(make_pair(initLeftBound, validStr));
    return;
    /*
    vector<size_t> overlapIndex;
    auto tempIter=iter;
    tempIter++;
    for(size_t i=1;i<overlapNum;++i){
        overlapIndex.push_back((*tempIter).first);
    }
    string mergedStr(mergedLength, '-');
    size_t start;
    if(iter!=this->unassembledBuf.end())
        start=(*iter).first;
    else
        start=index;

    size_t offset=0;
    for(size_t i=0;i<overlapNum;++i){
        offset=(*iter).first-start;
        string tempStr=(*iter).second;
        for(size_t j=0;j<tempStr.length();++j){
            mergedStr[offset+j]=tempStr[j];
        }
        ++iter;
    }
    for(size_t i=0;i<overlapIndex.size();++i){
        this->unassembledBuf.erase(overlapIndex[i]);
    }
    return mergedStr;*/
}

void StreamReassembler::deleteOverlap(map<size_t, string>::iterator iter, size_t count){
    vector<size_t> deleteIndex;
    for(size_t i=0;i<count;++i){
        deleteIndex.push_back((*iter).first);
        ++iter;
    }
    for(size_t i=0;i<count;++i){
        this->unassembledBuf.erase(deleteIndex[i]);
    }
}

// 待更新
void StreamReassembler::insertStr(const string &data, size_t index){
    size_t leftOverlap=0;
    size_t rightOverlap=0;
    size_t mergedLength=0;
    size_t oldLength=0;

    auto leftIter=this->detectOverlap(index, data, leftOverlap, rightOverlap, mergedLength, oldLength);
    string mergedStr;
    this->merge(index, data, leftIter, leftOverlap, rightOverlap, mergedLength);
    /*
    if(leftOverlap+rightOverlap!=0){
        // 若重叠的区间数量不为0
        if(leftOverlap!=0)
            // 若左重叠数量不为0，可以删除index对应的元素
            this->unassembledBuf.erase(index);
        else{
            // 若左重叠数量为0，需要更新iter
            start=(*leftIter).first;
            ++leftIter;
        }
        mergedStr=this->merge(index, data, leftIter, leftOverlap, rightOverlap, mergedLength);
    }
    else
        // 若无重叠的内容
        mergedStr=data;
    this->unassembledSize+=(mergedLength-oldLength);
    this->insertToHeap(index);
    if(leftOverlap==0&&rightOverlap!=0)
        // 在这个情况下，buf中index对应的iter被删除，需要重新插入
        this->unassembledBuf.insert(make_pair(start, mergedStr));
    else
        // 在这个情况下，iter不会被删除，直接替换它对应的值即可
        (*leftIter).second=mergedStr;
    */
}

void StreamReassembler::assembleStr(){
    //cout<<"期望的index: "<<this->expectedIdx<<"目前的最小索引: "<<this->peek()<<endl;
    while(!(this->idx_heap.empty())&&this->expectedIdx>=this->peek()){
        size_t peekVal=this->pop();
        if(this->unassembledBuf.find(peekVal)==this->unassembledBuf.end())
            // 若在未组装区中没有这个peekVa
            return;
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

void StreamReassembler::refreshSize(){
    size_t temp=0;
    for(auto iter=this->unassembledBuf.begin();iter!=this->unassembledBuf.end();++iter)
        temp+=(*iter).second.length();
    this->unassembledSize=temp;
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