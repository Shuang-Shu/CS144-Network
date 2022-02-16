#include "timer.hh"
using namespace std;

Timer::Timer(size_t aRTO):
  totalTimePassed(0)
, initRTO(aRTO)
, RTO(initRTO)
, retransCount(0)
, running(false){}

// 重设Timer的参数，但不改变运行状态
void Timer::reset(){
    totalTimePassed=0;
    // initRTO
    RTO=initRTO;
    retransCount=0;
}

// 判断是否超时
bool Timer::isEtire(){
    if(!running)
        return false;
    if(totalTimePassed>=RTO){
        running=false;
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

// 将RTO翻倍
void Timer::doubleRTO(){
    RTO*=2;
    retransCount+=1;
}

// 打开计时器
void Timer::open(){
    running=true;
}

// 关闭计时器
void Timer::close(){
    running=false;
}

// 检查计时器是否正在运行
bool Timer::isRunning(){
    return running;
}

// 重启计时器
void Timer::restart(){
    totalTimePassed=0;
    open();
}

// 获取重传次数
uint8_t Timer::getRetransCount() const{
    return retransCount;
}