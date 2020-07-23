
#ifndef PHISYCAL_LAYER_H
#define PHISYCAL_LAYER_H


#include <string.h>
#include "Arduino.h"

// determines packet size in bytes
#define PKT_SIZE	1

#define CONNECT		73


// packet definition
typedef struct {
	unsigned char data[PKT_SIZE];
} packet;


// frames are transported in this layer
typedef struct {
	unsigned char kind;		// what kind of frame is it?
	unsigned char seq;		// sequence number
	unsigned char ack;		// acknowledgement number
	packet info;			// the network layer packet
	unsigned char checksum;
} frame;


//static bool is_connected __attribute__ ((section (".noinit")));


class PhysicalLayer {
	private:
		int read_bytes(unsigned char* buff, unsigned int len);
		int read_frames(unsigned char* buff, unsigned int len);
		int write_frames(unsigned char* buff, unsigned int len);

	public:
		unsigned long get_tick();
		int init(unsigned long baudrate);
		int connect(char type);
		int recv(frame* f, unsigned int len);
		int send(frame* f, unsigned int len);
		int end();
		void flush(unsigned long timeout);
};


#endif
