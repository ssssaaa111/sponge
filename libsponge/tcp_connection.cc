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
    auto origin_state = state();
    if (state() != TCPState::State::TIME_WAIT)
    {
        // std::cerr<<"not time_wait:"<<get_status(state())<< std::endl;
       _sender.set_time_has_waited(0);
    }
    
     if (seg.header().rst)
     {
         std::cerr<<"get a rst: close  connect" <<std::endl;
         _sender.stream_in().set_error();
         _receiver.stream_out().set_error();
         _active = 0;
         _linger_after_streams_finish = 0;
         std::cerr<<"14after seg_recieved:"<<get_status(state())<< std::endl;
         return;
     }
     
     // resend fin
     if ((state() == TCPState::State::TIME_WAIT || state() == TCPState::State::CLOSING)  && seg.header().fin)
    {
        // TCPSegment seg1;
        // seg1.header().ack = true;
        // _sender.set_time_has_waited(0);
        // seg1.header().ackno = seg.header().seqno + 1;
        // _segments_out.push(seg1);
        _receiver.segment_received(seg);
         if(_sender.segments_out().empty() && _receiver.ackno().has_value() && (seg.payload().size() > 0 ||seg.header().fin))
        {
                std::cerr<<"CLOSE_WAIT to send a empty seg"<< std::endl;
                _sender.set_time_has_waited(0);
                _sender.send_empty_segment1();
        }
        push_seg_out();
         std::cerr<<"send ack to client:"<<get_status(state())<<"  ackno:"<< seg.header().seqno + 1 << std::endl;
        return;
    }
    
    if (state() == TCPState::State::SYN_SENT)
        {
            std::cerr<<" from SYN_SENT"<<std::endl;
            // TCPSegment seg1;
            // seg1.header().ack = true;
            // seg1.header().ackno = seg.header().seqno + 1;
            // seg1.header().win = _cfg.recv_capacity;
            // _segments_out.push(seg1);
            _receiver.segment_received(seg);
            _sender.ack_received(seg.header().ackno, seg.header().win);
            if (seg.header().syn)
            {
                std::cerr<<" get syn "<<std::endl;
                _sender.send_empty_segment1();
            }
            push_seg_out();
            
            std::cerr<<" SYN_SENT change to: "<<get_status(state())<<std::endl;
        }

    if (seg.header().ack)
    { 
        // std::cerr<<"get ack with akcno :"<<seg.header().ackno<< std::endl;
        if (seg.header().fin)
        {
            // std::cerr<<"get fin in :"<<get_status(state())<< std::endl;
        }
        
        if (state() == TCPState::State::SYN_RCVD)
        {
            // std::cerr<<"1111111166666666666666666666666:"<< seg.header().win <<std::endl;
            // std::cerr<<"1111111166666666666666666666666:"<< seg.header().ackno.raw_value() <<std::endl;
            // std::cerr<<"1111111166666666666666666666666:"<< _sender.next_seqno().raw_value() <<std::endl;

            if (seg.header().win > 0)
            {
                if (seg.header().win <= 1)
                {
                    // std::cerr<<"win_size1->"<< seg.header().win <<std::endl;
                }
                
                _sender.ack_received(seg.header().ackno, seg.header().win);
            }
            if (seg.header().ackno == _sender.next_seqno())
            {
                
                
                // std::cerr<<_sender.next_seqno_absolute() <<" "<< _sender.bytes_in_flight()<<std::endl;
                _sender.set_bytes_in_flight(0);
                // std::cerr<<"from SYN_RCVD to->>>>" << get_status(state()) << std::endl;;
                // TODO::next_abs_sqno is need to add one?
                // std::cerr<<"12after seg_recieved:"<<get_status(state())<< std::endl;
                return;
            }
            
        }
        
        if (state() == TCPState::State::LISTEN)
        {
            std::cerr<<"in listen state: next abs sqo is ->"<<_sender.next_seqno_absolute()<<std::endl;
            // TCPSegment seg1;
            // seg1.header().ack = true;
            // _linger_after_streams_finish = false;
            // seg1.header().ackno = seg.header().seqno + 1;
            // _segments_out.push(seg1);
        //  std::cerr<<"11after seg_recieved:"<<get_status(state())<< std::endl;
            return;
        }
        
        if (state() == TCPState::State::CLOSE_WAIT)
        {
            _sender.set_time_has_waited(0);
            _receiver.segment_received(seg);
            _sender.ack_received(seg.header().ackno, seg.header().win);
            if(_sender.segments_out().empty() && _receiver.ackno().has_value() && (seg.payload().size() > 0 ||seg.header().fin))
            {
                    std::cerr<<"CLOSE_WAIT to send a empty seg"<< std::endl;
                    _sender.send_empty_segment1();
            }
            push_seg_out();

            std::cerr<<"change CLOSE_WAIT to"<<get_status(state())<< std::endl;

            // if (seg.header().fin)
            // {
            //     TCPSegment seg1;
            //     seg1.header().ack = true;
            //     _linger_after_streams_finish = false;
            //     seg1.header().ackno = seg.header().seqno + 1;
            //     _segments_out.push(seg1);
            //     _receiver.segment_received(seg);
            //     std::cerr<<"from: CLOSE_WAIT(get fin) change to: "<<get_status(state())<< std::endl;
            // }
        }

        if (state() == TCPState::State::TIME_WAIT)
        {
            _sender.set_time_has_waited(0);
            _receiver.segment_received(seg);
            _sender.ack_received(seg.header().ackno, seg.header().win);
            if(_sender.segments_out().empty() && _receiver.ackno().has_value() && (seg.payload().size() > 0 ||seg.header().fin))
            {
                    std::cerr<<"TIME_WAIT to send a empty seg"<< std::endl;
                    _sender.send_empty_segment1();
            }
            push_seg_out();
        }

        if (state() == TCPState::State::ESTABLISHED)
        {
            // auto origin_state = state();
            // if (seg.header().fin)
            // {
            //     TCPSegment seg1;
            //     seg1.header().ack = true;
            //     _linger_after_streams_finish = false;
            //     seg1.header().ackno = seg.header().seqno + 1;
            //     // std::cerr<<"send ack to client ackno:" <<seg.header().seqno + 1<< std::endl;
            //     _segments_out.push(seg1);
            //     _receiver.segment_received(seg);
            //     std::cerr<<get_status(origin_state)<< " change to "<<":"<<get_status(state())<< " get fin and send ackno:"<<seg.header().seqno + 1<<std::endl;
            //     return;
            // }
            // else {
                // bool need_send_ack = seg.length_in_sequence_space();
                // TCPSegment seg1;
                // seg1.header().ack = true;
                // std::cerr<<"-------need send back ack"<<std::endl;
                // std::cerr<< "receiver ack_no is->"<<_receiver.ackno().value() << std::endl;
                // std::cerr<< "seg sqn is->"<<seg.header().seqno << std::endl;
                // std::cerr<< "sender next sqn->"<<_sender.next_seqno() << std::endl;
                // std::cerr<< "seg ackno is->"<<seg.header().ackno << std::endl;
                // std::cerr<< "seg win is->"<<seg.header().win << std::endl;
            _receiver.segment_received(seg);
            _sender.ack_received(seg.header().ackno, seg.header().win);
            if (_receiver.stream_out().input_ended())
            {
                _linger_after_streams_finish = false;
            }
            
            if ((seg.payload().size() > 0 ||seg.header().fin) && _sender.segments_out().empty() && _receiver.ackno().has_value())
            {
                
                _sender.send_empty_segment1();
            }
            push_seg_out();
            // std::cerr<<"is sync recieve:"<<_sender.next_seqno_absolute() <<" : "<< _sender.stream_in().bytes_written()<< std::endl;
            // std::cerr<<"=======need send back ack"<<std::endl;
            // seg1.header().win = _receiver.window_size();
            // seg1.header().ackno = _receiver.ackno().value();
            // std::cerr<< "established after ack_no is->"<<_receiver.unwrap_sqn(_receiver.ackno().value()) << std::endl;
            // _segments_out.push(seg1); 
            //  std::cerr<<"win_size3->"<< seg1.header().win <<std::endl;
            std::cerr<<"from: ESTABLISHED change to: "<<get_status(state())<<" bytes in the fly:"<< _sender.bytes_in_flight()<< std::endl;
            return;
            // }
        }

        if (state() == TCPState::State::LAST_ACK)
        {
            if ((seg.header().ackno - _sender.next_seqno()) >= 0)
            {
                std::cerr<<"before current byte in fly:"<<_sender.bytes_in_flight()<<std::endl;
                _receiver.segment_received(seg);
                _sender.ack_received(seg.header().ackno, seg.header().win);
                // std::cerr<<"current byte in fly:"<<_<<std::endl;
                // for (size_t i = 0; i < _sender.bytes_in_flight(); i++)
                // {
                //     _sender.
                // }
                std::cerr<<"after current byte in fly:"<<_sender.bytes_in_flight()<<std::endl;

                // _sender.set_bytes_in_flight(0);
                if (_sender.bytes_in_flight() == 0)
                {
                    _active = 0;
                }
                
                std::cerr<<"current last ack, change to" << get_status(state())<<std::endl;
            } else {
                std::cerr<<"current last ack, but bad ackno:"<<seg.header().ackno <<" but need "<<_sender.next_seqno()<<std::endl;
                std::cerr<<"current last ack, ackno diff:"<<seg.header().ackno - _sender.next_seqno()<<std::endl;
                std::cerr<<"current last ack, bytes on fly:"<<_sender.bytes_in_flight()<<std::endl;
            }
        }
        
       
        if (state() == TCPState::State::FIN_WAIT_1)
        {
            std::cerr<<"in State::FIN_WAIT_1 get seg size:"<<seg.payload().size()<<std::endl;
        //     // std::cerr<<seg.header().ackno << " -> "<<_sender.next_seqno()<<std::endl;
        //     if (seg.header().fin && seg.header().ackno == _sender.next_seqno())
        //     {
        //         std::cerr<<"FIN_WAIT_1 change to TIME_WAIT"<<std::endl;
        //         // std::cerr<<"before seg out size:" << _segments_out.size() <<std::endl;
        //         _sender.set_bytes_in_flight(0);
        //         _sender.set_time_has_waited(0);
        //         TCPSegment seg1;
        //         seg1.header().ack = true;
        //         seg1.header().ackno = seg.header().seqno + 1;
        //         _segments_out.push(seg1);
        //         _receiver.segment_received(seg);
        //         // std::cerr<<"after seg out size:" << _segments_out.size() <<std::endl;
        // //  std::cerr<<"8after seg_recieved:"<<get_status(state())<< std::endl;
        //         return;
        //     }

        //     if (seg.header().ackno == _sender.next_seqno())
        //     {
        //         std::cerr<<"FIN_WAIT_1 change to FIN_WAIT_2"<<std::endl;
        //         // _sender.set_bytes_in_flight(0);
        //         _sender.set_time_has_waited(0);
        //         // no need to send anything
        //         // TCPSegment seg1;
        //         // seg1.header().ack = true;
        //         // seg1.header().ackno = seg.header().seqno + 1;
        //         // _segments_out.push(seg1);
        //         _receiver.segment_received(seg);
        //         _sender.ack_received(seg.header().ackno, seg.header().win);
        //         // std::cerr<<"change from fin_wait_1 to :"<<get_status(state())<< std::endl;
        //         return;
        //     }
        //     if (seg.header().fin)
        //     {
        //         auto state_origin = state(); 
        //         TCPSegment seg1;
        //         seg1.header().ack = true;
        //         seg1.header().ackno = seg.header().seqno + 1;
        //         _segments_out.push(seg1);
        //         _receiver.segment_received(seg);
        //         std::cerr<<get_status(state_origin)<<" change to State::Closing" <<" get fin and send ackno:"<<seg.header().seqno + 1<<std::endl;
        // //  std::cerr<<"6after seg_recieved:"<<get_status(state())<< std::endl;
        //         return;
        //     }
            _sender.set_time_has_waited(0);
            _receiver.segment_received(seg);
            _sender.ack_received(seg.header().ackno, seg.header().win);
            if(_sender.segments_out().empty() && _receiver.ackno().has_value() && (seg.payload().size() > 0 ||seg.header().fin))
            {
                    std::cerr<<"FIN_WAIT_1 to send a empty seg"<< std::endl;
                    _sender.send_empty_segment1();
            }
            push_seg_out();

            std::cerr<<"change FIN_WAIT_1 to"<<get_status(state())<< std::endl;
            // std::cerr<<"missed::should get a fin "<<seg.header().fin<<" state:"<<get_status(state())<<std::endl;
            // std::cerr<<"missed::should get a fin ackno: "<<seg.header().ackno<<" expect:"<<_sender.next_seqno()<<std::endl;
            // std::cerr<<"missed::should get a fin ackno diff: "<<seg.header().ackno - _sender.next_seqno()<<std::endl;
            // std::cerr<<"missed::should get a fin, bytes in the fly: "<<seg.header().ackno<<" expect:"<<_sender.bytes_in_flight()<<std::endl;
        
        }

        if (state() == TCPState::State::FIN_WAIT_2)
        {
            std::cerr<<"in State::FIN_WAIT_2 get seg size:"<<seg.payload().size()<<std::endl;
            auto init_ackno = _receiver.ackno();

            // if (seg.header().fin)
            // {
            // auto origin_state = state();
            _sender.set_time_has_waited(0);
            // TCPSegment seg1;
            // seg1.header().ack = true;
            // seg1.header().ackno = seg.header().seqno + 1;
            // _segments_out.push(seg1);
            _receiver.segment_received(seg);
            _sender.ack_received(seg.header().ackno, seg.header().win);
            
            if((seg.payload().size() > 0 ||seg.header().fin) && _sender.segments_out().empty() && _receiver.ackno().has_value())
            {
                std::cerr<<"FIN_WAIT_2 to send a empty seg, is seg has fin:"<< seg.header().fin << std::endl;
                _sender.send_empty_segment1();
            }
            push_seg_out();
            std::cerr<<"in State::FIN_WAIT_2 get ack diff:"<<_receiver.ackno().value() - init_ackno.value()<<std::endl;
            std::cerr<<"in State::FIN_WAIT_2 unassembled bytes: "<<_receiver.unassembled_bytes() << "<------"<<std::endl;
            std::cerr<<"from FIN_WAIT_2 change to "<<":"<<get_status(state())<<std::endl;
            // std::cerr<<"seg out size:" << _segments_out.size() <<std::endl;
            // std::cerr<<"seg ackno:" << seg1.header().ackno <<std::endl;
            //  std::cerr<<"from " << get_status(origin_state)<<" change to "<<get_status(state())<< std::endl;
            return;
            // }
            
        }

        if (state() == TCPState::State::CLOSING)
        {
            // auto state_origin = state(); 
            _sender.set_time_has_waited(0);
            // _sender.set_bytes_in_flight(0);
            _receiver.segment_received(seg);
            _sender.ack_received(seg.header().ackno, seg.header().win);
             std::cerr<<"from CLOSING change to: "<<get_status(state())<<std::endl;
            return;
        }

        if (state() == TCPState::State::SYN_RCVD)
        {
            _sender.set_bytes_in_flight(0);
        //  std::cerr<<"3after seg_recieved:"<<get_status(state())<< std::endl;
            return;
        }
        
        //  if (state() == TCPState::State::SYN_SENT && seg.header().syn)
         
        
        _sender.ack_received(seg.header().ackno, seg.header().win);
        //  std::cerr<<get_status(state())<<" win_size4->"<< seg.header().win <<std::endl;
        seg.header().ackno;
        _receiver.segment_received(seg);
        push_seg_out();

        std::cerr<<"bytes in the fly:"<<_sender.bytes_in_flight()<<std::endl;        
        std::cerr<<get_status(origin_state)<<" change(last) to: "<<get_status(state())<<std::endl;        
        // cout<<"state:"<< state().name()<< endl;
        // if (!get_status(state()).compare("error status"))
        // {
          
        // }
        
    }

    if (state() == TCPState::State::LISTEN)
        {
            // std::cerr<<"listen"<<std::endl;
        //     if (seg.header().syn)
        //     {
        //         TCPSegment seg1;
        //         seg1.header().ack = true;
        //         seg1.header().syn = true;
        //         seg1.header().seqno = _sender.next_seqno();
        //         seg1.header().ackno = seg.header().seqno + 1;
        //         seg1.header().win = _cfg.recv_capacity;
        //         _segments_out.push(seg1);
        //         _sender.set_next_seqno_absolute(1);
        //         _sender.set_bytes_in_flight(1);
        //         _sender.set_next_seqno(1);
        //         _receiver.segment_received(seg);
        //         // _sender.ack_received(seg.header().ackno, seg.header().win);
        //         // std::cerr<<"after seg out size:" << _segments_out.size() <<std::endl;
        // //  std::cerr<<"2after seg_recieved:"<<get_status(state())<< std::endl;
        //         return;       
        //     }
            _receiver.segment_received(seg);
            _sender.ack_received(seg.header().ackno, seg.header().win);
            push_seg_out();
            std::cerr<<"change LISTEN to "<<get_status(state())<< std::endl;
            return;
        }

    // if (state() == TCPState::State::SYN_SENT && seg.header().syn)
    // {
    //     // TCPSegment seg1;
    //     // seg1.header().ack = true;
    //     // seg1.header().ackno = seg.header().seqno + 1;
    //     // _segments_out.push(seg1);
    //     _receiver.segment_received(seg);
    // }
    

     

    // std::cerr<<" after seg_recieved:"<<state().name()<< std::endl;
    push_seg_out();
    std::cerr<<"1after seg_recieved:"<<get_status(state())<< std::endl;
 }

bool TCPConnection::active() const { 
    return _active; 
}

void TCPConnection::push_seg_out() {
    // std::cerr<<"push seg out sender status is: "<<get_status(state()) << std::endl;
    // if (TCPSenderStateSummary::FIN_SENT == TCPState::state_summary(_sender))
    // {
    //     TCPSegment seg;
    //     seg.header().ack = true;
    //     seg.header().ackno = _receiver.ackno().value();
    //     seg.header().win = _receiver.window_size();
    //     _segments_out.push(seg);
    //     return;
    // }
    // else if (TCPSenderStateSummary::FIN_ACKED == TCPState::state_summary(_sender)) {
    //     TCPSegment seg;
    //     seg.header().ack = true;
    //     seg.header().ackno = _receiver.ackno().value();
    //     seg.header().win = _receiver.window_size();
    //     _segments_out.push(seg);
    //     return;
    // }    
    //  if(state() == TCPState::State::SYN_RCVD) {
    //     TCPSegment seg;
    //     seg.header().ack = true;
    //     seg.header().ackno = _receiver.ackno().value();
    //     seg.header().win = _receiver.window_size();
    //     _segments_out.push(seg);
    //     return;
    // } else {
        _sender.fill_window();
        // if (_sender.segments_out().size() == 0 && state() == TCPState::State::ESTABLISHED)
        // {
        //      _sender.send_empty_segment1();
        // }
        
    // }
    std::cerr<< "sender.segments_out().size():"<< _sender.segments_out().size()<<std::endl;
    while (_sender.segments_out().size() != 0)
    {
        auto seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            std::cerr<< "push add _receiver ackno:"<<  _receiver.unwrap_sqn(_receiver.ackno().value())<<std::endl;
            if (_receiver.window_size() == 0)
            {
              std::cerr<< "_receiver size:"<<  _receiver.window_size()<<std::endl;
            }
            
            seg.header().win = _receiver.window_size();
        }
        _segments_out.push(seg);

    }
    
}

size_t TCPConnection::write(const string &data) {
    // DUMMY_CODE(data);
    if (TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_SENT)
    {
        // std::cerr<<get_status(state()) <<" can't write any data anymore........" <<std::endl;
        return 0;
    }
    
    size_t size = _sender.stream_in().write(data);
    std::cerr<< "write into len:" << size<<" status is: "<<get_status(state()) << std::endl;
    // _sender.fill_window();
    push_seg_out();
    return size;

    
    // return {};
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    // DUMMY_CODE(ms_since_last_tick); 
    // std::cerr<<"seg out size  before tick:" << _segments_out.size() <<std::endl;
    if(state() == TCPState::State::ESTABLISHED) {
        // std::cerr<<_sender.consecutive_retransmissions() << "-> "<<  TCPConfig::MAX_RETX_ATTEMPTS <<std::endl;
        if (_sender.consecutive_retransmissions() >= TCPConfig::MAX_RETX_ATTEMPTS)
        {
            // std::cerr<<"excceed max retx attempts"<<std::endl;
            _sender.stream_in().set_error();
            _receiver.stream_out().set_error();
            _active = false;
            _linger_after_streams_finish = false;
            TCPSegment seg;
            seg.header().rst = true;
            _segments_out.push(seg);
            return;
        }
        
    }

    _sender.tick(ms_since_last_tick);
    while (_sender.segments_out().size() != 0)
    {
        auto seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
            if (_receiver.window_size() <= 1) {
               std::cerr<<"receiver win size is out"<< "-> "<<  _receiver.window_size() <<std::endl;
            }
        }
        if (_receiver.ackno().has_value()) {
            std::cerr<<get_status(state())<<" resend push seg ackno:"<< "-> "<< _receiver.unwrap_sqn(_receiver.ackno().value())  <<std::endl;
        }
        std::cerr<<get_status(state())<<" resend push seg sqn:"<< "-> "<< seg.header().seqno  <<std::endl;
        std::cerr<<get_status(state())<<" resend push seg is fin:"<< "-> "<<seg.header().fin <<std::endl;
        _segments_out.push(seg);

    }
    // std::cerr<<"seg out size  after tick:" << _segments_out.size() <<std::endl;
    
    // std::cerr<<_sender.consecutive_retransmissions() << "--" << _sender.init_transmissions() << std::endl;
    // std::cerr<<"have waited for:"<<_sender.time_has_waited() << "--" << _sender.init_transmissions() <<" end-----"<<std::endl;
    // std::cerr<<"tick:"<< ms_since_last_tick<<" " <<get_status(state()) <<" end-----"<<std::endl;
     if ((state() == TCPState::State::TIME_WAIT 
            || state() == TCPState::State::CLOSE_WAIT 
            || state() == TCPState::State::LAST_ACK) 
            && ms_since_last_tick > 0)
        {
            // std::cerr<<get_status(state()) <<" end-----"<<std::endl;
            // std::cerr<<get_status(state())<<":time passed:"<<ms_since_last_tick<<":has_waited:"<<_sender.time_has_waited() << ":init wait:" << _sender.init_transmissions()<<" sqn:"<< _sender.next_seqno() << std::endl;
            
        }
    // std::cerr<<"tick:"<<get_status(state())<< std::endl;
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
        // if (state() == TCPState::State::FIN_WAIT_1)
        // {
        //     std::cerr<<"stuck in fin wait 1.................."<<std::endl;
        //     TCPSegment seg;
        //     seg.header().ack = true;
        //     seg.header().ackno = _receiver.ackno().value();
        //     seg.header().fin = true;
        //     seg.header().win = _receiver.window_size();
        //     seg.header().seqno = _sender.next_seqno()-1;
        //     _segments_out.push(seg);
        //     return;
        // }

        // if (state() == TCPState::State::LAST_ACK)
        // {
        //     // std::cerr<<"stuck in LAST_ACK.................."<<std::endl;
        //     TCPSegment seg;
        //     seg.header().ack = true;
        //     seg.header().ackno = _receiver.ackno().value();
        //     seg.header().fin = true;
        //     seg.header().seqno = _sender.next_seqno()-1;
        //     _segments_out.push(seg);
        //     return;
        // }
        
    }
    

    if (_sender.time_has_waited() == 4 * _sender.init_transmissions())
    {
         if (state() == TCPState::State::CLOSING)
        {
            // std::cerr<<"stuck in CLOSING.................."<<std::endl;
            // TCPSegment seg;
            // seg.header().ack = true;
            // seg.header().ackno = _receiver.ackno().value();
            // seg.header().fin = true;
            // seg.header().seqno = _sender.next_seqno();
            // _segments_out.push(seg);
        }

        
    }
    // std::cerr<<"seg out size  after tick:" << _segments_out.size() <<std::endl;
    
    }

void TCPConnection::end_input_stream() {
    std::cerr<<"before end_input_stream:"<<get_status(state())<< std::endl;
    _sender.stream_in().end_input();
    TCPSegment seg;
    string init_state =  get_status(state());
    // if (state() == TCPState::State::ESTABLISHED)
    // {
        
    //     seg.header().ack = true;
    //     seg.header().ackno = _receiver.ackno().value();
    //     seg.header().fin = true;
    //     seg.header().seqno = _sender.next_seqno();
    //     seg.header().win = _receiver.window_size();
    //     // _sender.next_seqno
    //     _sender.send_empty_segment();
    //     // _sender.segments_on_going().push(seg);
    //     _sender.set_bytes_in_flight(1);
    //     _segments_out.push(seg);
    //     _sender.set_time_has_waited(0);
    //    std::cerr<<"active end, send a fin to server, sqn:"<<_sender.next_seqno()<< std::endl;
    // }

    // else if (state() == TCPState::State::CLOSE_WAIT)
    // {
    //     // _sender.fill_window();
    //     seg.header().ack = true;
    //     seg.header().ackno = _receiver.ackno().value();
    //     seg.header().fin = true;
    //     seg.header().seqno = _sender.next_seqno();
    //     // _sender.next_seqno
    //     _sender.send_empty_segment();
    //     // _sender.segments_on_going().push(seg);
    //     _sender.set_bytes_in_flight(1);
    //     _segments_out.push(seg);
    //     _sender.set_time_has_waited(0);
    //    std::cerr<<"passive end, send a fin to server, sqn:"<<_sender.next_seqno()<< std::endl;
    // } else {
    //     // std::cerr<<"error, should not in: "<<get_status(state())<< std::endl;
    //     return;
    // }
    push_seg_out();
   
    std::cerr<<"is sync recieve:"<<(_sender.next_seqno_absolute() < (_sender.stream_in().bytes_written()+2)) << " finshed"<< std::endl;
    std::cerr<<"is sync recieve:"<<_sender.next_seqno_absolute() <<" : "<< _sender.stream_in().bytes_written()<< std::endl;
    std::cerr<<"is sync recieve:"<<_sender.next_seqno_absolute() <<" : "<< _sender.stream_in().bytes_read()<< std::endl;
    std::cerr<<"after end of stream:" <<init_state <<" change to: "<<get_status(state())<< std::endl;

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
    // std::cerr<<"status:"<<"in_syn_recv"<<std::endl;
    return _receiver.ackno().has_value() && !_receiver.stream_out().input_ended(); 
    }


    
TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            std::cerr<<"bad disconnection:" <<get_status(state())<<endl;
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            _receiver.stream_out().set_error();
            _sender.stream_in().set_error();
            _active = false;
            TCPSegment seg;
            seg.header().rst = true;
            _segments_out.push(seg);

        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
