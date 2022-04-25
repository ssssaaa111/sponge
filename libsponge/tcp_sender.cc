#include "tcp_sender.hh"

#include "tcp_config.hh"
#include<iostream>
#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const {
     return _bytes_in_flight; 
    }

void TCPSender::fill_window() {
    TCPSegment seg;
    // 还未开始发送，先设置syn位
    if (_next_seqno == 0)
    {
        seg.header().syn = true;
    }
    seg.header().seqno = _isn + _next_seqno;
    size_t len = seg.length_in_sequence_space();
    string payload = _stream.read(_window_size-len);
    // if (payload.size() == 0)
    // {
    //     return;
    
    // }
    
    seg.payload() = std::string(payload);
    std::cout<<"bytes_written:"<<_stream.bytes_written()<<std::endl;
    std::cout<<"bytes_read:"<<_stream.bytes_read()<<std::endl;
    std::cout<<"is stream eof:"<<_stream.eof()<<std::endl;
     if (seg.length_in_sequence_space() == 0)
    {
        return;
    }
    
    _segments_out.push(seg);
    _next_seqno += _window_size;
   
    _bytes_in_flight += seg.length_in_sequence_space();
}

// TCPSegment TCPSender::build_segment(string data, ) {
//         TCPSegment seg;
//         seg.payload() = std::string(data);
//         seg.header().ack = ack;
//         seg.header().fin = fin;
//         seg.header().syn = syn;
//         seg.header().rst = rst;
//         seg.header().ackno = ackno;
//         seg.header().seqno = seqno;
//         seg.header().win = win;
//         return seg;
//     }

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    
    WrappingInt32 last_ackno = _ackno;
    _ackno = ackno;
    _window_size = window_size;
    int32_t diff = (_ackno - last_ackno);
    std::cout<<"last_ack:"<<last_ackno.raw_value()<<std::endl;
    std::cout<<"new_ack:"<<_ackno.raw_value()<<std::endl;
    std::cout<<"diff:"<<diff<<std::endl;
    _bytes_in_flight -= diff;

}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }

unsigned int TCPSender::consecutive_retransmissions() const { return {}; }

void TCPSender::send_empty_segment() {}
