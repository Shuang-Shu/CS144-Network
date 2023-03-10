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
    push_to_stream();
    if (eof) {
        _eof_index = index + data.length();
    }
    size_t set_in_count = 0;
    size_t offset = index - _unassembled.get_head_index_in_stream();
    for (size_t i = 0; i < data.length(); i++) {
        if (i + index < _unassembled.get_head_index_in_stream()||offset+i>=_capacity)
            continue;
        if (_unassembled.set_at_index_zero(offset + i, data[i]))
            set_in_count++;
    }
    _unassembled_bytes = _unassembled_bytes + set_in_count;
    // push assembled char in _unassembled to _assembled
    if (index <= _expect) {
        auto head_length = _unassembled.peek_head_length();
        for (size_t i = 0; i < head_length; i++) {
            _assembled.push(_unassembled.pop_head());
        }
        _unassembled_bytes -= head_length;
        _assembled_bytes += head_length;
        _expect += head_length;
    }
    // try to send assembled chars
    push_to_stream();
    // close the output stream when the last char has arrived
    if (_expect == _eof_index && _unassembled_bytes == 0 && _assembled_bytes == 0) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }

bool StreamReassembler::push_to_stream() {
    bool output_sufficient = true;
    do {
        if (_assembled.size() == 0)
            break;
        auto head = _assembled.front();
        string temp(CHAR_LENGTH, head);
        if (_output.write(temp) == 0) {
            output_sufficient = false;
        } else {
            _assembled_bytes--;
            _assembled.pop();
        }
    } while (output_sufficient);
    return output_sufficient;
}