#include "timer.hh"
using namespace std;

Timer::Timer(uint64_t init_rto):time_passed(0),rto(init_rto), retran_number(0), is_open(false){}

void Timer::add_time(uint64_t new_time_passed){
    time_passed+=new_time_passed;
}

bool Timer::timeout(){
    if(time_passed>=rto){
        return true;
    }else{
        return false;
    }
}

void Timer::double_rto(){
    rto*=2;
    time_passed=0;
}

void Timer::reset(uint64_t init_rto){
    retran_number=0;
    time_passed=0;
    rto=init_rto;
}

void Timer::open(){
    is_open=true;
}

void Timer::close(){
    is_open=false;
}

void Timer::increase_retran_number(){
    retran_number++;
}

bool Timer::over_retrans(uint64_t reTransLimit){
    if(retran_number>reTransLimit)
        return true;
    return false;
}

int Timer::get_retran_number() const{
    return retran_number;
}