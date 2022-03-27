#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    DUMMY_CODE(n, isn);
    uint64_t largeNumber = n + isn.raw_value();
    uint32_t smallNumber = largeNumber & 0xFFFFFFFF;
    return WrappingInt32{smallNumber};
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
    DUMMY_CODE(n, isn, checkpoint);
    uint64_t prefix = checkpoint & 0xFFFFFFFF00000000;
    uint64_t relate_sqn = 0;
    if (n.raw_value() >= isn.raw_value())
    {
        relate_sqn = n.raw_value() - isn.raw_value();
    } else {
        relate_sqn = 0x100000000 - (isn.raw_value() - n.raw_value());
    }
    
    // 没有突破32位
    if (checkpoint < 0xFFFFFFFF) {
        uint64_t right = relate_sqn +0x100000000;
        if (checkpoint <= relate_sqn)
        {
            return relate_sqn;
        }
        
        if ((checkpoint - relate_sqn) < (right - checkpoint))
        {
            return relate_sqn;
        } else {
            return right;   
        }
        
        
    } else
    {
        uint64_t conbain = prefix + relate_sqn;
        if (conbain > checkpoint)
        {
            if (conbain - checkpoint < checkpoint - (conbain -0x100000000)) 
            {
                return conbain;
            } else {
                return conbain - 0x100000000;
            }
            
        } else {
            if (conbain + 0x100000000 - checkpoint < checkpoint - conbain) 
            {
                return conbain + 0x100000000;
            } else {
                return conbain;
            }
        }
    }
    return {};
}
