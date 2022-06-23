#include <random>
using namespace std;
class Timer{
    private:
        // 距离上次重传/收到确认经过的时间
        uint64_t time_passed;
        // 重传超时时间(ms)
        uint64_t rto;
        // 连续重传次数
        uint64_t retran_number;
        bool is_open;
    public:
        Timer(uint64_t);
        // 增加经过的时间
        void add_time(uint64_t);
        // 检测是否超时
        bool timeout();
        // 将rto加倍，此时time_passed也归0
        void double_rto();
        // 重设计时器，不改变开关状态，需要传入初始rto
        void reset(uint64_t);
        // 打开计时器
        void open();
        // 关闭计时器
        void close();
        // 增加连续重传次数
        void increase_retran_number();
        // 检测是否超过约定重传次数
        bool over_retrans(uint64_t);
};