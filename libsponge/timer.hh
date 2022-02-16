#include <random>
using namespace std;
class Timer{
    private:
        // 距离上次重传/收到确认经过的时间
        size_t totalTimePassed;
        // 重传超时时间(ms)
        uint8_t RTO;
        uint8_t reTransCount;
        bool running;
    public:
        Timer(size_t aRTO);
        void setRTO(uint8_t aRTO);
        bool isEtire();
        void addTime(size_t timePassed);
        void reset();
        void open();
        void close();
        bool isOverReTrans(int);
};