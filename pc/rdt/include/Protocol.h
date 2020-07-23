
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "PhysicalLayer.h"


/**
 * Maximum sequence number should be 2^n - 1
 */
#define MAX_SEQ			7


/**
 * Sender's/receiver's window size
 * This should be greater than half the number of sequence numbers
 */
#define WINDOW_SIZE		((MAX_SEQ + 1) / 2)


/**
 * Max number of buffered frames
 */
#define QUEUE_SIZE		(WINDOW_SIZE * 2)


/**
 * Possible frame kind
 */
#define ACK		1
#define NAK		2
#define DATA	3


/**
 * Macro inc is expanded in-line: Increment k circularly
 */
#define inc(k) if (k < MAX_SEQ) k = k + 1; else k = 0;


/**
 * Event type definition
 */
typedef enum {
	no_event = -1,
	frame_arrival = 0,
	cksum_err = 1,
	timeout = 2,
	send_ready = 3,
	ack_timeout = 4
} event_type;


/**
 * @brief      Class for protocol.
 * 
 * Implements a middleware between physical layer (read/write on serial)
 * and a ReliableDataTransfer implementations which offers different solutions
 * to make the transmission reliable
 * 
 */
class Protocol {

	private:

		bool error[MAX_SEQ + 1];

		PhysicalLayer physical_layer;
		int status;										///< 0 is disabled, 1 is enabled

		unsigned long long offset;						///< to prevent multiple timeouts on same tick
		unsigned long long ack_timer[WINDOW_SIZE];		///< ack timers
		unsigned long long lowest_timer;				///< lowest of the timers
		unsigned long long aux_timer;					///< value of the auxiliary timer

		unsigned char seqs[WINDOW_SIZE];				///< last sequence number sent per timer
		unsigned char oldest_frame;						///< tells which frame timed out

		frame queue[QUEUE_SIZE];						///< buffered incoming frames
		frame *inp = &queue[0];							///< where to put the next frame
		frame *outp = &queue[0];						///< where to remove the next frame from
		int nframes = 0;								///< number of queued frames

		frame last_frame;								///< arrive frames are kept here

		unsigned long long timeout_interval;			///< timeout interval from user
		unsigned char next_pkt_fetch;					///< seq of next packet from user to fetch
		unsigned char last_pkt_given;					///< seq of last pkt delivered to user

	public:

		/**
		 * @brief      Check if the value is between the extremes.
		 *
		 * @param[in]  a     The lower bound.
		 * @param[in]  b     The value included.
		 * @param[in]  c     The upper bound.
		 *
		 * @return     True if a <=b < c circularly; false otherwise.
		 */
		bool between(unsigned char a, unsigned char b, unsigned char c);

		/**
		 * @brief      Sets the maximum seqnr.
		 *
		 * @param[in]  max_seqnr  The maximum seqnr.
		 */
		void set_max_seqnr(unsigned char max_seqnr);

		/**
		 * @brief      Gets the timedout seqnr.
		 *
		 * @return     The timedout seqnr.
		 */
		unsigned char get_timedout_seqnr(void);

		/**
		 * @brief      Sets the timeout.
		 *
		 * @param[in]  timeout  The timeout.
		 */
		void set_timeout(unsigned long long timeout);

		/**
		 * @brief      Allow the application layer to cause a send_ready event.
		 */
		void enable_protocol(void);

		/**
		 * @brief      Forbid the application layer from causing a send_ready event.
		 */
		void disable_protocol(void);

		/**
		 * @brief      Set up the protocol to new send or receive.
		 *
		 * @param[in]  max_seqnr  The maximum seqnr
		 * @param[in]  timeout    The timeout
		 * @param[in]  state      The state
		 */
		void set_up(unsigned char max_seqnr, unsigned long long timeout, int state);

		/**
		 * @brief      Calculates the checksum.
		 *
		 * @param      data       The data
		 * @param[in]  num_bytes  The number bytes
		 *
		 * @return     The checksum.
		 */
		unsigned char compute_checksum(unsigned char data[], size_t num_bytes);

		/**
		 * @brief      Verify the checksum with the received data and checksum.
		 *
		 * @param      data       The data
		 * @param[in]  num_bytes  The number bytes
		 * @param[in]  checksum   The checksum
		 *
		 * @return     The result of checking, 0 if positive.
		 */
		unsigned char verify_checksum(unsigned char data[], size_t num_bytes, unsigned char checksum);

		/**
		 * @brief      Init the physical layer.
		 *
		 * @param[in]  device    The device
		 * @param[in]  baudrate  The baudrate
		 *
		 * @return     1 if success, -1 otherwise
		 */
		int init(const char* device, int baudrate);

		/**
		 * @brief      Connect and synch sender and receiver.
		 *
		 * @param[in]  type  The type (sender or receiver)
		 *
		 * @return     1 if success, 0 otherwise
		 */
		int connect(char type);

		/**
		 * @brief      Close the physical layer
		 *
		 * @return     Positive number if success
		 */
		int close();

		/**
		 * @brief      Flush the RX serial buffer
		 */
		void flush(unsigned long long timeout);

		/**
		 * @brief      Read from physical file descriptor and insert frame in the queue
		 */
		void enqueue(void);

		/**
		 * @brief      Remove element from the queue and check if is not corrupted
		 *
		 * @return     The event type, that could be cksum_err in case of corrupted frame.
		 */
		event_type dequeue(void);

		/**
		 * @brief      Wait for an event to happen; return its type in event
		 *
		 * @param      event  The event
		 */
		void wait_for_event(event_type* event);

		/**
		 * @brief      Pick an event if any
		 *
		 * @return     The the event happened
		 */
		event_type pick_event(void);

		/**
		 * @brief      Starts a timer and enable the timeout event.
		 *
		 * @param[in]  seqnr  The sequence number of started imer
		 */
		void start_timer(unsigned char seqnr);

		/**
		 * @brief      Stops a timer and disable the timeout event.
		 *
		 * @param[in]  seqnr  The sequence number of started imer
		 */
		void stop_timer(unsigned char seqnr);

		/**
		 * @brief      Starts the acknowledge timer and enable the ack_timeout event.
		 */
		void start_ack_timer(void);

		/**
		 * @brief      Stops the acknowledge timer and disable the ack_timeout event.
		 */
		void stop_ack_timer(void);

		/**
		 * @brief      Check for possible timeout. If found, reset the timer.
		 *
		 * @return     Return the sequence number of timeout timer if any, -1 if no timeout
		 */
		int check_timers(void);

		/**
		 * @brief      Check for ack timeout. If found, reset the timer.
		 *
		 * @return     Return 1 if found, 0 otherwise
		 */
		int check_ack_timer(void);

		/**
		 * @brief      Find the lowest timer.
		 */
		void recalc_timers(void);

		/**
		 * @brief      Fetch a packet from the application layer for transmission on the channel
		 *
		 * @param      data  The data from application layer
		 * @param      p     The packet in which put the data
		 */
		void from_application_layer(unsigned char* data, packet* p);

		/**
		 * @brief      Deliver information from an inbound frame to the physical layer
		 *
		 * @param      data  The data buffer in which put the received data
		 * @param      p     The packet received from channel.
		 */
		void to_application_layer(unsigned char* data, packet* p);

		/**
		 * @brief      Go get an inbound frame from the physical layer and copy it to f
		 *
		 * @param      f     The frame copied
		 */
		void from_physical_layer(frame* f);

		/**
		 * @brief      Pass the frame to the physical layer for transmission
		 *
		 * @param      f     The frame passed to channel
		 */
		void to_physical_layer(frame* f);

};

#endif