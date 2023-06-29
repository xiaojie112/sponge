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
    uint64_t eachGroup = static_cast<uint64_t>(1) << 32;
    uint32_t move = (n+1) % eachGroup - 1;
    return isn + move;
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
    // DUMMY_CODE(n, isn, checkpoint);
    uint32_t maxSeqno = (static_cast<uint64_t>(1) << 32) - 1;
    uint64_t distance = 0;
    if(n - isn >= 0){
        distance = n - isn;
    }else{
        distance = maxSeqno - isn.raw_value() + n.raw_value() + 1;
    }

    // uint64_t beforeDistance = checkpoint < distance ? distance - checkpoint : checkpoint - distance;
    // // uint64_t curDistance = 
    // if(checkpoint <= distance)return distance;

    uint64_t count = checkpoint / ((static_cast<uint64_t>(1) << 32));
    distance += count * ((static_cast<uint64_t>(1) << 32));
    if(distance <= checkpoint){
        if(distance + ((static_cast<uint64_t>(1) << 32)) >= checkpoint){
            distance = distance + ((static_cast<uint64_t>(1) << 32)) - checkpoint < checkpoint - distance ? distance + ((static_cast<uint64_t>(1) << 32)) : distance;
        }
    }else{
        if(distance - ((static_cast<uint64_t>(1) << 32)) <= checkpoint){
            distance = checkpoint - (distance - ((static_cast<uint64_t>(1) << 32))) < distance - checkpoint ? (distance - ((static_cast<uint64_t>(1) << 32))) : distance;
        }
    }


    return distance;
}
