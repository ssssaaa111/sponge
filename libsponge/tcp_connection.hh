#ifndef SPONGE_LIBSPONGE_TCP_FACTORED_HH
#define SPONGE_LIBSPONGE_TCP_FACTORED_HH

#include "tcp_config.hh"
#include "tcp_receiver.hh"
#include "tcp_sender.hh"
#include "tcp_state.hh"

//! \brief A complete endpoint of a TCP connection
class TCPConnection {
  private:
    TCPConfig _cfg;
    TCPReceiver _receiver{_cfg.recv_capacity};
    TCPSender _sender{_cfg.send_capacity, _cfg.rt_timeout, _cfg.fixed_isn};

    //! outbound queue of segments that the TCPConnection wants sent
    std::queue<TCPSegment> _segments_out{};

    //! Should the TCPConnection stay active (and keep ACKing)
    //! for 10 * _cfg.rt_timeout milliseconds after both streams have ended,
    //! in case the remote TCPConnection doesn't know we've received its whole stream?
    bool _linger_after_streams_finish{true};
    bool _active = true;

  public:
    const std::string STATE[12] =
    {
        "LISTEN",   //!< Listening for a peer to connect
        "SYN_RCVD",     //!< Got the peer's SYN
        "SYN_SENT",     //!< Sent a SYN to initiate a connection
        "ESTABLISHED",  //!< Three-way handshake complete
        "CLOSE_WAIT",   //!< Remote side has sent a FIN, connection is half-open
        "LAST_ACK",     //!< Local side sent a FIN from CLOSE_WAIT, waiting for ACK
        "FIN_WAIT_1",   //!< Sent a FIN to the remote side, not yet ACK'd
        "FIN_WAIT_2",   //!< Received an ACK for previously-sent FIN
        "CLOSING",      //!< Received a FIN just after we sent one
        "TIME_WAIT",    //!< Both sides have sent FIN and ACK'd, waiting for 2 MSL
        "CLOSED",       //!< A connection that has terminated normally
        "RESET", 
    };

    //! \name "Input" interface for the writer
    //!@{

    //! \brief Initiate a connection by sending a SYN segment
    void connect();

    std::string get_status(TCPState s) {
      if (s == TCPState::State::LISTEN)
      {
        return STATE[0];
      }

      if (s == TCPState::State::SYN_RCVD)
      {
        return STATE[1];
      }

      if (s == TCPState::State::SYN_SENT)
      {
        return STATE[2];
      }

      if (s == TCPState::State::ESTABLISHED)
      {
        return STATE[3];
      }
      
      if (s == TCPState::State::CLOSE_WAIT)
      {
        return STATE[4];
      }
      if (s == TCPState::State::LAST_ACK)
      {
        return STATE[5];
      }
      if (s == TCPState::State::FIN_WAIT_1)
      {
        return STATE[6];
      }
      if (s == TCPState::State::FIN_WAIT_2)
      {
        return STATE[7];
      }
      if (s == TCPState::State::CLOSING)
      {
        return STATE[8];
      }
      if (s == TCPState::State::TIME_WAIT)
      {
        return STATE[9];
      }

      if (s == TCPState::State::CLOSED)
      {
        return STATE[10];
      }

      if (s == TCPState::State::RESET)
      {
        return STATE[11];
      }

    return "error status";
    }

    //! \brief Write data to the outbound byte stream, and send it over TCP if possible
    //! \returns the number of bytes from `data` that were actually written.
    size_t write(const std::string &data);

    //! \returns the number of `bytes` that can be written right now.
    size_t remaining_outbound_capacity() const;

    //! \brief Shut down the outbound byte stream (still allows reading incoming data)
    void end_input_stream();
    //!@}

    //! \name "Output" interface for the reader
    //!@{

    //! \brief The inbound byte stream received from the peer
    ByteStream &inbound_stream() { return _receiver.stream_out(); }
    //!@}

    //! \name Accessors used for testing

    //!@{
    //! \brief number of bytes sent and not yet acknowledged, counting SYN/FIN each as one byte
    size_t bytes_in_flight() const;
    //! \brief number of bytes not yet reassembled
    size_t unassembled_bytes() const;
    //! \brief Number of milliseconds since the last segment was received
    size_t time_since_last_segment_received() const;
    //!< \brief summarize the state of the sender, receiver, and the connection
    TCPState state() const { return {_sender, _receiver, active(), _linger_after_streams_finish}; };
    //!@}

    //! \name Methods for the owner or operating system to call
    //!@{

    //! Called when a new segment has been received from the network
    void segment_received(const TCPSegment &seg);

    // as it said, push seg out 
    void push_seg_out();
    
    // syn sent stat check
    bool in_syn_sent();

    bool in_syn_recv();

    //! Called periodically when time elapses
    void tick(const size_t ms_since_last_tick);

    //! \brief TCPSegments that the TCPConnection has enqueued for transmission.
    //! \note The owner or operating system will dequeue these and
    //! put each one into the payload of a lower-layer datagram (usually Internet datagrams (IP),
    //! but could also be user datagrams (UDP) or any other kind).
    std::queue<TCPSegment> &segments_out() { return _segments_out; }

    //! \brief Is the connection still alive in any way?
    //! \returns `true` if either stream is still running or if the TCPConnection is lingering
    //! after both streams have finished (e.g. to ACK retransmissions from the peer)
    bool active() const;
    //!@}

    //! Construct a new connection from a configuration
    explicit TCPConnection(const TCPConfig &cfg) : _cfg{cfg} {}

    //! \name construction and destruction
    //! moving is allowed; copying is disallowed; default construction not possible

    //!@{
    ~TCPConnection();  //!< destructor sends a RST if the connection is still open
    TCPConnection() = delete;
    TCPConnection(TCPConnection &&other) = default;
    TCPConnection &operator=(TCPConnection &&other) = default;
    TCPConnection(const TCPConnection &other) = delete;
    TCPConnection &operator=(const TCPConnection &other) = delete;
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_FACTORED_HH
