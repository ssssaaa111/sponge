#include "tcp_sender.hh"

#include "tcp_config.hh"
#include<iostream>
#include <random>
#include<algorithm>

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
    , _retransmission_timeout{retx_timeout}
    , _retransmission_timeout_left{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const {
     return _bytes_in_flight; 
    }

void TCPSender::fill_window() {
    TCPSegment seg;
    size_t win = _window_size > 0 ? _window_size : 1;
    // if (_window_size == 0)
    // {
    //     _window_size = 1;
    // }
    
    // 还未开始发送，先设置syn位
    if (_next_seqno == 0)
    {
        seg.header().syn = true;
        seg.header().seqno = _isn;
        _segments_out.push(seg);
        _bytes_in_flight += seg.length_in_sequence_space();
        _next_seqno += 1;
        _segments_on_going.push(seg);
        // _window_size -= 1;
        _timer = true;
        return;
    }
    
    // uint16_t remainder_size = _window_size;
    size_t max_size = TCPConfig::MAX_PAYLOAD_SIZE;
    if (win + _ack_unwrap <= _next_seqno)
    {
        return;
    }
    
    
    
    while (true)
    {
        size_t remain_size_can_be_fill = win + _ack_unwrap - _next_seqno;
        if (_fin || remain_size_can_be_fill == 0)
        {
            break;
        }

        seg.header().seqno = wrap(_next_seqno, _isn); 
        string payload = _stream.read(min(remain_size_can_be_fill, max_size));
        std::cout<<"pushing->"<<payload.size()<<" chars...."<<std::endl;
        remain_size_can_be_fill -= payload.size();
        //  _window_size -=  payload.size();
        _fin = _stream.eof();
        if (_fin)
        {
            if (remain_size_can_be_fill > 0)
            {
                 seg.header().fin = _fin;
                // _window_size -= 1;
                // remain_size_can_be_fill -= 1;
            } else {
               _fin = false; 
            }
        }

        seg.payload() = std::move(payload);
        
        if (seg.length_in_sequence_space() == 0)
        {
            // std::cout<<"nothing to send....."<<std::endl;
            break;
        }
        // if (_fin && seg.length_in_sequence_space() == 1 && _bytes_in_flight > 0)
        // {
        //     _window_size += 1;
        //     _fin = false;
        //     break;
        // }
        
        _next_seqno += seg.length_in_sequence_space();
        _bytes_in_flight += seg.length_in_sequence_space();
        _segments_out.push(seg);
        _segments_on_going.push(seg);
        _timer = true;
    }
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
    
    // WrappingInt32 last_ackno = _ackno;
    uint64_t ack_unwrap = unwrap(ackno, _isn, _last_received_ack);
    if (_next_seqno < ack_unwrap) {
        return;
    }
    _window_size = window_size;
    if (ack_unwrap > _last_received_ack)
    {
        _ackno = ackno;
        _ack_unwrap = ack_unwrap;
        while (_segments_on_going.size() > 0)
        {
            // std::cout<<"front len:"<<_segments_on_going.front().length_in_sequence_space()<<std::endl;
            uint64_t current_end_sqn = unwrap(_segments_on_going.front().header().seqno, _isn, _last_received_ack) + 
                                        _segments_on_going.front().length_in_sequence_space();
            if (ack_unwrap >= current_end_sqn)
            {   
                _bytes_in_flight -= _segments_on_going.front().length_in_sequence_space();
                // _segments_out.push(_segments_on_going.front());
                _segments_on_going.pop();
                // _window_size = window_size;
                // if (_fin)
                // {
                //     _fin = false;
                // }
                
            } else {
                break;
            }
        }

        // 处理tick的各种值
        _retransmission_timeout = _initial_retransmission_timeout;
        _time_has_waited = 0;
        std::cout<<"_time_has_waited2:" << _time_has_waited<< std::endl;
        _retransmission_timeout_left = _initial_retransmission_timeout;
        if (_segments_on_going.size() > 0)
        {
            _timer = true;
        } else {
            _timer = false;
        }
        _consecutive_retransmissions = 0;
        _last_received_ack = ack_unwrap;
    }
    // fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _time_has_waited += ms_since_last_tick;
    if (_timer)
    {
        std::cout<<"_time_has_waited1:" << _time_has_waited<< std::endl;
        if (_retransmission_timeout_left <= ms_since_last_tick)
        {
            _segments_out.push(_segments_on_going.front());
            _consecutive_retransmissions += 1;
            if (_window_size != 0)
            {
                _retransmission_timeout *=2;
            }
            
            
            _retransmission_timeout_left = _retransmission_timeout;
        }
        else {
            _retransmission_timeout_left -= ms_since_last_tick;
        }
    }
    
     
     
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

unsigned int TCPSender::init_transmissions() const { return _initial_retransmission_timeout; }

void TCPSender::send_empty_segment() {
    _next_seqno += 1;
}
