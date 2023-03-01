#include "cycle_array.hh"

#include <iostream>

CycleArray::CycleArray(size_t cap) : _capacity{cap}, _head{0}, _head_index_in_stream{0}, _array{new char[cap]}, _null_array{new bool[cap]} {
    for(size_t i=0;i<cap;i++){
        _null_array[i]=true;
    }
}

size_t CycleArray::_logic_index_to_real(size_t logic_index) {
    if (logic_index >= _capacity) {
        std::cout << "index out of capacity" << std::endl;
        throw 0;
    }
    return ((_head + logic_index) % _capacity);
}

bool CycleArray::get_is_null(size_t logic_index){
    auto real_index=_logic_index_to_real(logic_index);
    return _null_array[real_index];
}

char CycleArray::get_at_index(size_t logic_index) {
    auto index = _logic_index_to_real(logic_index);
    return _array[index];
}

void CycleArray::set_index_null(size_t logic_index) {
    auto index = _logic_index_to_real(logic_index);
    _null_array[index]=true;
}

bool CycleArray::set_at_index_zero(size_t logic_index, char c) {
    auto index = _logic_index_to_real(logic_index);
    if (_null_array[index] ==true) {
        _array[index] = c;
        _null_array[index]=false;
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
    while (head_length<_capacity&&get_is_null(head_length++) != true) {
    }
    if(head_length==_capacity&&get_is_null(head_length-1)!=true){
        return head_length;
    }
    return head_length - 1;
}