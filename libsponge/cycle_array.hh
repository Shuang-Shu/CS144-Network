#ifndef CYCLE_QUEUE_HH
#define CYCLE_QUEUE_HH
typedef unsigned long size_t;

class CycleArray {
  private:
    size_t _capacity;
    size_t _head; // head of queue
    // size_t _tail; // tail of queue(not refer to a element)
    size_t _head_index_in_stream;
    char *_array;
    bool *_null_array;
    // functions

    // convert login index to real index
    size_t _logic_index_to_real(size_t logic_index);
    // convert real index to logic
    size_t _real_index_to_logic(size_t real_index); 

  public:
    CycleArray(size_t cap);
    // return the char at logic index
    char get_at_index(size_t);
    // return is nall at logic index
    bool get_is_null(size_t);
    void set_index_null(size_t);
    // set char at logic index when there is zero
    bool set_at_index_zero(size_t l_index, char c);
    // set value of element to 0 from l_index, until meet 0, then reset _head
    char pop_head(); 
    size_t peek_head_length(); 
    size_t get_head_index_in_stream(); 
};
#endif