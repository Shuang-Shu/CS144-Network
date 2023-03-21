#ifndef TIMER_HH
#define TIMER_HH

#include <cstdint>
class Timer {
  private:
    uint64_t _rto;
    uint64_t _initial_rto;
    uint64_t _time_pass;
    uint16_t _fail_count;
    bool _running;
    // shift of _initial_retransmission_timeout
    // current_isn=_initial_retransmission_timeout<<_retransmission_timeout_shift
    unsigned int _retransmission_timeout_shift{0};

  public:
    Timer(uint64_t _init_rto)
        : _rto(_init_rto)
        , _initial_rto(_init_rto)
        , _time_pass(0)
        , _fail_count(0)
        , _running(false)
        , _retransmission_timeout_shift(0) {}

    // call this function to change time
    void pass(uint64_t);
    // stop the timer
    void stop();
    // start the timer
    void start();
    // reset the timer
    void reset();
    // check whether the timer is expire
    bool is_expire();
    // get the continous fail count
    uint16_t fail_count() const;
};
#endif