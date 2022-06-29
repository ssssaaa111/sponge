#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const {
    // _sender
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const { 
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const { 
    return _receiver.unassembled_bytes();
 }

size_t TCPConnection::time_since_last_segment_received() const { return {}; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    //  DUMMY_CODE(seg);
     if (seg.header().rst)
     {
         _sender.stream_in().set_error();
         _receiver.stream_out().set_error();
         return;
     }
    if (seg.header().ack)
    {
        _sender.ack_received(seg.header().ackno, seg.header().win);
        seg.header().ackno;
        _receiver.segment_received(seg);
    }

    push_seg_out();
 }

bool TCPConnection::active() const { 
    return _active; 
}

void TCPConnection::push_seg_out() {
    if(in_syn_sent()) {
        TCPSegment seg;
        seg.header().ack = true;
        seg.header().ackno = _receiver.ackno().value();
        _segments_out.push(seg);
        return;
    } else {
        _sender.fill_window();
    }
    while (_sender.segments_out().size() != 0)
    {
        auto seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        _segments_out.push(seg);

    }
    
}

size_t TCPConnection::write(const string &data) {
    // DUMMY_CODE(data);
    
    size_t size = _sender.stream_in().write(data);
    _sender.fill_window();
    return size;

    
    // return {};
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    // DUMMY_CODE(ms_since_last_tick); 
    _sender.tick(ms_since_last_tick);
    }

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
}

void TCPConnection::connect() {
    // _sender.fill_window();
    push_seg_out();
}

bool TCPConnection::in_syn_sent() {
   return  _sender.next_seqno_absolute() > 0 && 
   _sender.next_seqno_absolute() == _sender.bytes_in_flight();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
