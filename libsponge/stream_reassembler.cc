#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity)
    , _capacity(capacity)
    , _unassembled{CycleArray(capacity)}
    , _assembled(queue<char>())
    , _assembled_bytes{0}
    , _unassembled_bytes{0}
    , _expect{0}
    , _eof_index{} {
    _eof_index = 1 << 31;
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (eof) {
        _eof_index = index + data.length();
    }
    if (data.length() + _unassembled_bytes + _assembled_bytes > _capacity) {
        return;
    }
    if (index + data.length() > _unassembled.get_head_index_in_stream() + _capacity) {
        return;
    }
    size_t set_in_count = 0;
    for (size_t i = 0; i < data.length(); i++) {
        size_t offset = index - _unassembled.get_head_index_in_stream();
        if (_unassembled.set_at_index_zero(offset + i, data[i]))
            set_in_count++;
    }
    _unassembled_bytes++;
    // push assembled char in _unassembled to _assembled
    if (index == _expect) {
        auto head_length = _unassembled.peek_head_length();
        // char *buffer = new char[head_length];
        for (size_t i = 0; i < head_length; i++) {
            // buffer[i] = _unassembled.pop_head();
            _assembled.push(_unassembled.pop_head());
        }
        // string seg=string(buffer, buffer+head_length);
        // _assembled.push(seg);
        // _output.write(seg);
        _unassembled_bytes -= head_length;
        _assembled_bytes += head_length;
    }
    // try to send assembled chars
    bool output_sufficient = true;
    do {
        auto head = _assembled.front();
        string temp(CHAR_LENGTH, head);
        if (_output.write(temp) == 0) {
            output_sufficient = false;
        } else {
            _assembled_bytes--;
            _assembled.pop();
        }
    } while (output_sufficient);
    // close the output stream when the last char has arrived
    if (_expect == _eof_index && _unassembled_bytes == 0 && _assembled_bytes == 0) {
        _output.end_input();
    }
    // do{
    //     auto head=_assembled.front();
    //     auto head_length=head.length();
    //     auto write_chars=_output.write(head);
    //     if(write_chars<head_length){

    //     }else{
    //         _assembled.pop();
    //         _assembled_bytes-=head_length;
    //     }
    // }while(output_sufficientv)
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }
