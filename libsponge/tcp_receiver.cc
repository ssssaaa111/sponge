#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    _len = seg.length_in_sequence_space();
    if (_is_started) {
        if (seg.header().syn) {
            return;
        }
    } else if (seg.header().syn) {
        _init_sqn = seg.header().seqno;
        _is_started = true;
        _len -= 1;
    } else {
        // 表示没接到syn，未开始
        return;
    }

    seg.header().seqno;
    if (_len > 0 || seg.header().fin) {
        uint64_t stream_index = unwrap(seg.header().seqno, _init_sqn, _checkpoint);
        // 检查下这个stream index 是不是0 且不为syn, 非法stream index
        if (stream_index == 0 && !seg.header().syn)
        {
            return;
        }
        
        if (stream_index > 0) {
            stream_index -= 1;
        }
        _reassembler.push_substring(seg.payload().copy(), stream_index, seg.header().fin);
        // head index 属于 stream index，不包括syn，所以这里-1和+1抵消
        _last_checkpoint = _checkpoint;
        _checkpoint = _reassembler.head_index();
        // stream index 不包括fin
        // 是否到了最后一位fin
        if (seg.header().fin)
        {
            _len -= 1;
            _is_fin = true;
            if (_reassembler.empty())
            {
                 _checkpoint += 1;
            }
        }
        
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_is_started) {
        // 这里需要+1，因为stream index从零开始不包括syn 和 fin
        if (_is_fin && _reassembler.empty())
        {     
            return WrappingInt32(wrap( _reassembler.head_index()+2, _init_sqn));  
        }
        return WrappingInt32(wrap( _reassembler.head_index()+1, _init_sqn));
    }
    return {};
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
