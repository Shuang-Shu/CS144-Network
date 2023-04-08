#include "stream_reassembler.hh"
#include <iostream>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

CycleArray::CycleArray(size_t cap)
    : _capacity{cap}, _head{0}, _head_index_in_stream{0}, _array{new char[cap]}, _null_array{new bool[cap]} {
    for (size_t i = 0; i < cap; i++) {
        _null_array[i] = true;
    }
}

size_t CycleArray::_logic_index_to_real(size_t logic_index) {
    if (logic_index >= _capacity) {
        cerr << "index out of capacity" << endl;
        throw 0;
    }
    return ((_head + logic_index) % _capacity);
}

bool CycleArray::get_is_null(size_t logic_index) {
    auto real_index = _logic_index_to_real(logic_index);
    return _null_array[real_index];
}

char CycleArray::get_at_index(size_t logic_index) {
    auto index = _logic_index_to_real(logic_index);
    return _array[index];
}

void CycleArray::set_index_null(size_t logic_index) {
    auto index = _logic_index_to_real(logic_index);
    _null_array[index] = true;
}

bool CycleArray::set_at_index_zero(size_t logic_index, char c) {
    auto index = _logic_index_to_real(logic_index);
    if (_null_array[index] == true) {
        _array[index] = c;
        _null_array[index] = false;
        return true;
    }
    return false;
}

char CycleArray::pop_head() {
    char r = get_at_index(0);
    set_index_null(0);
    _head = (_head + 1) % _capacity;
    _head_index_in_stream++;
    return r;
}

size_t CycleArray::get_head_index_in_stream() { return this->_head_index_in_stream; }

size_t CycleArray::peek_head_length() {
    size_t head_length = 0;
    while (head_length < _capacity && get_is_null(head_length++) != true) {
    }
    if (head_length == _capacity && get_is_null(head_length - 1) != true) {
        return head_length;
    }
    return head_length - 1;
}

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
        if (i + index < _unassembled.get_head_index_in_stream() || offset + i >= _capacity)
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