#include <random>
using namespace std;
class Timer{
    private:
        // 距离上次重传/收到确认经过的时间
        size_t totalTimePassed;
        // 初始重传时间
        uint64_t initRTO;
        // 重传超时时间(ms)
        uint8_t RTO;
        uint8_t retransCount;
        bool running;
    public:
        Timer(size_t aRTO);
        void doubleRTO();
        bool isEtire();
        bool isRunning();
        void addTime(size_t timePassed);
        void reset();
        void restart();
        void open();
        void close();
        uint8_t getRetransCount() const;
};