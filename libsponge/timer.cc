#include "timer.hh"
using namespace std;

Timer::Timer(size_t aRTO):totalTimePassed(0),RTO(aRTO), reTransCount(0), running(false){}

// 重设Timer的时间
void Timer::reset(){
    totalTimePassed=0;
    running=false;
}

// 判断是否超时
bool Timer::isEtire(){
    if(totalTimePassed>=RTO){
        return true;
    }else{
        return false;
    }
}

// 增加时间
void Timer::addTime(size_t passTime){
    if(!running)
        return;
    totalTimePassed+=passTime;
}

// 设置重传超时时间
void Timer::setRTO(uint8_t newRTO){
    RTO=newRTO;
}

// 打开计时器
void Timer::open(){
    running=true;
}

// 关闭计时器
void Timer::close(){
    running=false;
}

// 检查连续重传是否超出次数
bool Timer::isOverReTrans(int reTransLimit){
    if(reTransCount>reTransLimit)
        return true;
    return false;
}