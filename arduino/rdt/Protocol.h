#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "PhysicalLayer.h"


// maximum sequence number should be 2^n - 1
#define MAX_SEQ			7

// sender's/receiver's window size (this should be greater than half the number of sequence numbers)
#define WINDOW_SIZE		((MAX_SEQ + 1) / 2)

// max number of buffered frames
#define QUEUE_SIZE		(WINDOW_SIZE * 2)

// possible frame kind
#define ACK		1
#define NAK		2
#define DATA	3


// Macro inc is expanded in-line: Increment k circularly
#define inc(k) if (k < MAX_SEQ) k = k + 1; else k = 0;


// event type definition
typedef enum {
	no_event = -1,
	frame_arrival = 0,
	cksum_err = 1,
	timeout = 2,
	send_ready = 3,
	ack_timeout = 4
} event_type;



class Protocol {

	private:

		bool error[MAX_SEQ + 1];

		PhysicalLayer physical_layer;
		int status;										// 0 is disabled, 1 is enabled

		// Timer
		unsigned long offset;							// to prevent multiple timeouts on same tick
		unsigned long ack_timer[WINDOW_SIZE];			// ack timers
		unsigned long lowest_timer;						// lowest of the timers
		unsigned long aux_timer;						// value of the auxiliary timer

		unsigned char seqs[WINDOW_SIZE];				// last sequence number sent per timer
		unsigned char oldest_frame;						// tells which frame timed out

		// Incoming frames are buffered here for later processing
		frame queue[QUEUE_SIZE];						// buffered incoming frames
		frame *inp = &queue[0];							// where to put the next frame
		frame *outp = &queue[0];						// where to remove the next frame from
		int nframes = 0;								// number of queued frames

		frame last_frame;								// arrive frames are kept here

		unsigned long timeout_interval;					// timeout interval from user
		unsigned char next_pkt_fetch;					// seq of next packet from user to fetch
		unsigned char last_pkt_given;					// seq of last pkt delivered to user

	public:

		bool between(unsigned char a, unsigned char b, unsigned char c);
		void set_max_seqnr(unsigned char max_seqnr);
		unsigned char get_timedout_seqnr(void);
		void set_timeout(unsigned long timeout);
		void enable_protocol(void);
		void disable_protocol(void);
		void set_up(unsigned char max_seqnr, unsigned long timeout, int state);

		unsigned char compute_checksum(unsigned char data[], unsigned int num_bytes);
		unsigned char verify_checksum(unsigned char data[], unsigned int num_bytes, unsigned char checksum);

		int init(unsigned long baudrate);
		int connect(char type);
		int close();
		void flush(unsigned long timeout);

		// Read from physical file descriptor and insert frame in the queue
		void enqueue(void);
		// Remove element from the queue
		event_type dequeue(void);

		// Wait inside call pick event until event is equal to no event
		void wait_for_event(event_type* event);
		event_type pick_event(void);

		// Timer function to manage timeout
		void start_timer(unsigned char seqnr);
		void stop_timer(unsigned char seqnr);
		void start_ack_timer(void);
		void stop_ack_timer(void);
		int check_timers(void);
		int check_ack_timer(void);
		void recalc_timers(void);

		// Take MAX_PKT bytes from data and send it in a frame
		void from_application_layer(unsigned char* data, packet* p);
		// Insert inside user buffer a MAX_PKT bytes at time
		void to_application_layer(unsigned char* data, packet* p);
		// Take last frame extracted by the queue
		void from_physical_layer(frame* f);
		// Write on physical file descriptor the frame
		void to_physical_layer(frame* f);

};

#endif
