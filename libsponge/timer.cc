#include "timer.hh"

void Timer::pass(uint64_t ms) {
    if (!_running) {
        return;
    }
    _time_pass += ms;
    if (_time_pass >= _rto) {
        _fail_count++;
        _time_pass = 0;
        _retransmission_timeout_shift++;
        _rto = _initial_rto << _retransmission_timeout_shift;
    }
}

void Timer::stop() {
    _running = false;
}

void Timer::start() {
    _running = true;
}

void Timer::reset() {
    _time_pass = 0;
    _fail_count = 0;
    _retransmission_timeout_shift = 0;
    _rto = _initial_rto;
}

bool Timer::is_expire() {
    return _time_pass >= _rto;
}

uint16_t Timer::fail_count() const {
    return _fail_count;
}