#ifndef CYCLE_QUEUE_HH
#define CYCLE_QUEUE_HH
typedef unsigned long size_t;

class CycleArray {
  private:
    size_t _capacity;
    size_t _head;
    size_t _head_index_in_stream;
    char *_array;
    // functions

    // convert login index to real index
    size_t _logic_index_to_real(size_t logic_index);

  public:
    CycleArray(size_t cap);
    char get_at_index(size_t);
    void set_at_index(size_t, char);
    // set char at logic index when there is zero
    bool set_at_index_zero(size_t l_index, char c);
    // set value of element to 0 from l_index, until meet 0, then reset _head
    char pop_head();
    char peek();
    size_t peek_head_length();
    size_t get_head_index_in_stream();
};
#endif