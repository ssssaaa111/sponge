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

size_t TCPConnection::time_since_last_segment_received() const { return {_sender.time_has_waited()}; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    //  DUMMY_CODE(seg);
    // std::cout<<"before seg_recieved:"<<state().name()<< std::endl;

    _sender.set_time_has_waited(0);

     if (seg.header().rst)
     {
         _sender.stream_in().set_error();
         _receiver.stream_out().set_error();
         _active = 0;
         _linger_after_streams_finish = 0;
         return;
     }
    
    if (seg.header().ack)
    {
        if (state() == TCPState::State::LISTEN)
        {
            // std::cout<<"in listen state: next abs sqo is ->"<<_sender.next_seqno_absolute()<<std::endl;
            // TCPSegment seg1;
            // seg1.header().ack = true;
            // _linger_after_streams_finish = false;
            // seg1.header().ackno = seg.header().seqno + 1;
            // _segments_out.push(seg1);
            return;
        }
        
        if (state() == TCPState::State::CLOSE_WAIT)
        {
            if (seg.header().fin)
            {
                TCPSegment seg1;
                seg1.header().ack = true;
                _linger_after_streams_finish = false;
                seg1.header().ackno = seg.header().seqno + 1;
                _segments_out.push(seg1);
                _receiver.segment_received(seg);
            }
        }

        if (state() == TCPState::State::ESTABLISHED)
        {
            if (seg.header().fin)
            {
                TCPSegment seg1;
                seg1.header().ack = true;
                _linger_after_streams_finish = false;
                seg1.header().ackno = seg.header().seqno + 1;
                _segments_out.push(seg1);
                _receiver.segment_received(seg);
            }
            else if(seg.payload().size() == 0){
                // TDDO::this is going to change......
                return;
            }
        }

        if (state() == TCPState::State::LAST_ACK)
        {
            if (seg.header().ackno == _sender.next_seqno())
            {
                // std::cout<<"current byte in fly:"<<_sender.bytes_in_flight()<<std::endl;
                _receiver.segment_received(seg);
                _sender.set_bytes_in_flight(0);
                _active = 0;
            }
        }
        
       
        if (state() == TCPState::State::FIN_WAIT_1)
        {
            // std::cout<<"in State::FIN_WAIT_1"<<std::endl;
            if (seg.header().fin && seg.header().ackno == _sender.next_seqno())
            {
                // std::cout<<"change to State::TIME_WAIT"<<std::endl;
                // std::cout<<"before seg out size:" << _segments_out.size() <<std::endl;
                _sender.set_bytes_in_flight(0);
                _sender.set_time_has_waited(0);
                TCPSegment seg1;
                seg1.header().ack = true;
                seg1.header().ackno = seg.header().seqno + 1;
                _segments_out.push(seg1);
                _receiver.segment_received(seg);
                // std::cout<<"after seg out size:" << _segments_out.size() <<std::endl;
                return;
            }

            if (seg.header().ackno == _sender.next_seqno())
            {
                // std::cout<<"change to State::FIN_WAIT_2"<<std::endl;
                _sender.set_bytes_in_flight(0);
                _sender.set_time_has_waited(0);
                // no need to send anything
                // TCPSegment seg1;
                // seg1.header().ack = true;
                // seg1.header().ackno = seg.header().seqno + 1;
                // _segments_out.push(seg1);
                _receiver.segment_received(seg);
                return;
            }
            
        }

        if (state() == TCPState::State::FIN_WAIT_2)
        {
            if (seg.header().fin)
            {
                TCPSegment seg1;
                seg1.header().ack = true;
                seg1.header().ackno = seg.header().seqno + 1;
                _segments_out.push(seg1);
                _receiver.segment_received(seg);
                // std::cout<<"seg out size:" << _segments_out.size() <<std::endl;
                // std::cout<<"seg ackno:" << seg1.header().ackno <<std::endl;
                return;
            }
            
        }

        if (state() == TCPState::State::CLOSING)
        {
            _sender.set_time_has_waited(0);
            _sender.set_bytes_in_flight(0);
            return;
        }
        _sender.ack_received(seg.header().ackno, seg.header().win);
        seg.header().ackno;
        _receiver.segment_received(seg);
        
    }

     

    // std::cout<<" after seg_recieved:"<<state().name()<< std::endl;
    push_seg_out();
 }

bool TCPConnection::active() const { 
    return _active; 
}

void TCPConnection::push_seg_out() {
    // std::cout<<"sender status is: "<<TCPState::state_summary(_sender) << std::endl;
    if (TCPSenderStateSummary::FIN_SENT == TCPState::state_summary(_sender))
    {
        TCPSegment seg;
        seg.header().ack = true;
        seg.header().ackno = _receiver.ackno().value();
        _segments_out.push(seg);
        return;
    }
    else if (TCPSenderStateSummary::FIN_ACKED == TCPState::state_summary(_sender)) {
        TCPSegment seg;
        seg.header().ack = true;
        seg.header().ackno = _receiver.ackno().value();
        _segments_out.push(seg);
        return;
    }    
    else if(in_syn_recv()) {
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
    // std::cout<<"seg out size  before tick:" << _segments_out.size() <<std::endl;

    _sender.tick(ms_since_last_tick);
    // std::cout<<_sender.consecutive_retransmissions() << "--" << _sender.init_transmissions() << std::endl;
    // std::cout<<"have waited for:"<<_sender.time_has_waited() << "--" << _sender.init_transmissions() << std::endl;
    
    // std::cout<<state().name()<< std::endl;
    if (_sender.time_has_waited() >= 10* _sender.init_transmissions())
    {
        
        if (state() == TCPState::State::TIME_WAIT)
        {
            _active = false;
            _linger_after_streams_finish = false;
        }
        
    }

    if (_sender.time_has_waited() >= _sender.init_transmissions())
    {
        if (state() == TCPState::State::FIN_WAIT_1)
        {
            // std::cout<<"stuck in fin wait 1.................."<<std::endl;
            TCPSegment seg;
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().fin = true;
            seg.header().seqno = _sender.next_seqno()-1;
            _segments_out.push(seg);
            return;
        }

        if (state() == TCPState::State::LAST_ACK)
        {
            // std::cout<<"stuck in fin wait 1.................."<<std::endl;
            TCPSegment seg;
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().fin = true;
            seg.header().seqno = _sender.next_seqno()-1;
            _segments_out.push(seg);
            return;
        }
        
    }
    

    if (_sender.time_has_waited() == 4 * _sender.init_transmissions())
    {
         if (state() == TCPState::State::CLOSING)
        {
            TCPSegment seg;
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().fin = true;
            seg.header().seqno = _sender.next_seqno();
            _segments_out.push(seg);
        }

        
    }
    // std::cout<<"seg out size  after tick:" << _segments_out.size() <<std::endl;
    
    }

void TCPConnection::end_input_stream() {
    // std::cout<<"before end_input_stream:"<<state().name()<< std::endl;
    _sender.stream_in().end_input();
    TCPSegment seg;
    seg.header().ack = true;
    seg.header().ackno = _receiver.ackno().value();
    seg.header().fin = true;
    seg.header().seqno = _sender.next_seqno();
    // _sender.next_seqno
    _sender.send_empty_segment();
    // _sender.segments_on_going().push(seg);
    _sender.set_bytes_in_flight(1);
    _segments_out.push(seg);
    _sender.set_time_has_waited(0);
    // std::cout<<"after end_input_stream:"<<state().name()<< std::endl;
    return;


}

void TCPConnection::connect() {
    // _sender.fill_window();
    push_seg_out();
}

bool TCPConnection::in_syn_sent() {
   return  _sender.next_seqno_absolute() > 0 && 
   _sender.next_seqno_absolute() == _sender.bytes_in_flight();
}

bool TCPConnection::in_syn_recv() { 
    // std::cout<<"status:"<<"in_syn_recv"<<std::endl;
    return _receiver.ackno().has_value() && !_receiver.stream_out().input_ended(); 
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
