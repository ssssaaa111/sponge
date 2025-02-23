#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include<iostream>
#include "wrapping_integers.hh"

#include <functional>
#include <queue>

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    std::queue<TCPSegment> _segments_on_going{};

    //! retransmission timer for the connection
    unsigned int _initial_retransmission_timeout;
    unsigned int _retransmission_timeout;
    unsigned int _retransmission_timeout_left;
    unsigned int _time_has_waited = 0;
    unsigned int _consecutive_retransmissions = 0;

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    bool _fin = false;

    bool _timer = false;


    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0};
    uint64_t _last_received_ack{0};
    uint64_t _bytes_in_flight{0};
    uint64_t _ack_unwrap{0};

    WrappingInt32 _ackno = _isn;
    
    uint16_t _window_size = 1;

    bool _fin_to_be_sent = false;

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    void send_empty_segment1() {
       // empty segment doesn't need store to outstanding queue
      TCPSegment seg;
      seg.header().seqno = wrap(_next_seqno, _isn);
      seg.header().ack = true;
      _segments_out.push(seg);
    };

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    void set_bytes_in_flight(uint64_t x) {
        _bytes_in_flight = x;
    };

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    unsigned int init_transmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }

    std::queue<TCPSegment> &segments_on_going() { return _segments_on_going; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    void set_next_seqno_absolute(uint64_t x ) { _next_seqno = x; }

    //! \brief time waited total
    uint64_t time_has_waited() const { return _time_has_waited; }

    void set_time_has_waited(uint64_t x) { 
      // std::cerr<<"_time_has_waited has been set to :"<< x << std::endl;
       _time_has_waited  = x; 
      }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }

    void set_next_seqno(uint64_t x) { _next_seqno = x; }
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
