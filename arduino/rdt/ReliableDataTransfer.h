
#ifndef RELIABLE_DATA_TRANSFER_H
#define RELIABLE_DATA_TRANSFER_H

#include "Protocol.h"

class ReliableDataTransfer {

	private:
		bool no_nak;						// no nak has been sent yet
		bool not_expected;
		bool end;							// To stop send and receive

		unsigned char ack_expected;			// lower edge of sender's window
		unsigned char next_frame_to_send;	// upper edge of sender's window + 1
		unsigned char frame_expected;		// lower edge of receiver's window
		unsigned char too_far;				// upper edge of receiver's window + 1

		frame r;							// scratch variable
		packet out_buf[WINDOW_SIZE];		// buffers for the outbound stream
		packet in_buf[WINDOW_SIZE];			// buffers for the inbound stream
		bool arrived[WINDOW_SIZE];			// inbound bit map
		unsigned int nbuffered;				// how many output buffers currently used

		event_type event;

		Protocol protocol;

		unsigned int nframes;				// Number of frames to send and receive
		unsigned int last_frame_recv;		// Counter for frame received
		unsigned int last_frame_send;		// Counter for frame send

		void (ReliableDataTransfer::*run)(unsigned char*);

		void set_up(int len);
		int connect(char type);
		void send_frame(unsigned char fk, unsigned char frame_nr, unsigned char frame_expected, packet buff[]);
		void selective_repeat(unsigned char* buff);
		void go_back_n(unsigned char* buff);

	public:

		int init(const char* protocol, unsigned long baudrate);
		void send(unsigned char* data, int len, unsigned long timeout);
		void recv(unsigned char* data, int len, unsigned long timeout);
		int close();
};


#endif
