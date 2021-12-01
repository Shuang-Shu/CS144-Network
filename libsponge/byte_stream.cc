#include "byte_stream.hh"
#include <deque>
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity): cap(capacity), is_end(false), total_bytes_written(0), total_bytes_read(0), buf(){ 
    // ByteStream的构造函数，在初始化列表中对成员进行初始化
}

size_t ByteStream::write(const string &data) {
    //DUMMY_CODE(data);
    if(this->is_end)
        return 0;
    size_t p=0;
    while(this->buf.size()<this->cap&&p<data.length()){
        this->buf.push_back(data[p]);
        p+=1;
    }
    this->total_bytes_written=this->total_bytes_written+p;
    return p;
}

//! \param[in] len bytes will be copied from the output side of the buffer
// 观察流接下来的 len 字节的内容
string ByteStream::peek_output(const size_t len) const {
    // DUMMY_CODE(len);
    string result=string(len, '-');
    size_t real_len=0;
    size_t index=0;
    real_len=this->buf.size()<len?this->buf.size():len;
    for(auto i=this->buf.begin();i<this->buf.begin()+real_len;++i){
        result[index]=*i;
        index+=1;
    }
    return result.substr(0, real_len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
     // DUMMY_CODE(len);
     size_t count=0;
     while(count<len&&!this->buf.empty()){
        this->buf.pop_front();
        count++;
     }
     this->total_bytes_read=this->total_bytes_read+count;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    // DUMMY_CODE(len);
    string result=ByteStream::peek_output(len);
    ByteStream::pop_output(len);
    // this->total_bytes_read=this->total_bytes_read+result.length();
    return result;
}

void ByteStream::end_input() {
    this->is_end=true;
}

bool ByteStream::input_ended() const {
     return this->is_end;
}

size_t ByteStream::buffer_size() const {
     return this->buf.size();
}

bool ByteStream::buffer_empty() const {
    return this->buf.empty();
}

bool ByteStream::eof() const {
    // 满足两个条件： 1.输入已经结束 2.缓冲区中已经无内容
     return this->buf.empty()&&this->is_end;
}

size_t ByteStream::bytes_written() const { return this->total_bytes_written; }

size_t ByteStream::bytes_read() const { return this->total_bytes_read; }

size_t ByteStream::remaining_capacity() const { return this->cap-this->buf.size(); }
