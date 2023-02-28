#include "cycle_array.hh"
#include <iostream>

CycleArray::CycleArray(size_t cap) : _capacity{cap}, _head{0}, _head_index_in_stream{0} { _array = new char[cap]; }

size_t CycleArray::_logic_index_to_real(size_t logic_index) {
    if (logic_index >= _capacity) {
        std::cout << "index out of capacity" << std::endl;
        throw 0;
    }
    return (_head + logic_index) % _capacity;
}

char CycleArray::get_at_index(size_t logic_index) {
    auto index = _logic_index_to_real(logic_index);
    return _array[index];
}

void CycleArray::set_at_index(size_t logic_index, char c) {
    auto index = _logic_index_to_real(logic_index);
    _array[index] = c;
}

bool CycleArray::set_at_index_zero(size_t logic_index, char c) {
    auto index = _logic_index_to_real(logic_index);
    if (_array[index] != 0) {
        _array[index] = c;
        return true;
    }
    return false;
}

char CycleArray::pop_head(){
    char r=get_at_index(0);
    set_at_index(0, 0);
    _head=(_head+1)%_capacity;
    _head_index_in_stream++;
    return r;
}

char CycleArray::peek(){
    return get_at_index(0);
}

size_t CycleArray::get_head_index_in_stream(){
    return this->_head_index_in_stream;
}

size_t CycleArray::peek_head_length(){
    size_t head_length=0;
    while (get_at_index(head_length++)!=0){}
    return head_length-1;
}