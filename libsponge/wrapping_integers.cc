#include "wrapping_integers.hh"

#include "iostream"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

static const uint64_t N = 1ll << 32;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint64_t sum = n + static_cast<uint64_t>(isn.raw_value());
    uint32_t res = static_cast<uint32_t>(sum % N);
    return WrappingInt32{res};
}

uint64_t dist(uint64_t a, uint64_t b) {
    if (b >= a) {
        // cout << "(b, a)=(" << b << ", " << a << ")" << endl;
        return b - a;
    } else {
        // cout << "(a, b)=(" << a << ", " << b << ")" << endl;
        return a - b;
    }
}

uint64_t cacul_result(uint64_t base, uint64_t n_64_abs, uint64_t isn_64) {
    auto value = base * N + n_64_abs - isn_64;
    // cout << "base: " << base << ", n:" << n_64_abs << ", isn:" << isn_64 << ", value: " << value << endl;
    return value;
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    auto n_64_abs = static_cast<uint64_t>(n.raw_value() - isn.raw_value());
    auto offset = checkpoint % N;
    auto base = checkpoint / N;
    if (n_64_abs >= offset && n_64_abs - offset >= N / 2 && checkpoint >= N) {
        base--;
    } else if (n_64_abs < offset && offset - n_64_abs >= N / 2) {
        base++;
    }
    return base * N + n_64_abs;
}
