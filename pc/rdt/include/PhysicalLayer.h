
#ifndef PHISYCAL_LAYER_H
#define PHISYCAL_LAYER_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>

/**
 * Determines packet size in bytes
 */
#define PKT_SIZE	1

/**
 * Bytes needed fo connection
 */
#define CONNECT		73


/**
 * Convert the user baudrate to termios baudrate
 */
struct rate {
	unsigned int rawrate;
	unsigned int termiosrate;
};



/**
 * The conversion table
 */
const rate conversiontable[] = {
	{9600, B9600},
	{19200, B19200},
	{38400, B38400},
	{57600, B57600},
	{115200, B115200}
};


/**
 * Packet definition
 */
typedef struct {
	unsigned char data[PKT_SIZE];
} packet;


/**
 * Frames are transported in this layer
 */
typedef struct {
	unsigned char kind;			///< What kind of frame is it?
	unsigned char seq;			///< Sequence number
	unsigned char ack;			///< Acknowledgement number
	packet info;				///< The data packet
	unsigned char checksum;		///< The checksum computed on data packet
} frame;



/**
 * @brief      Class for physical layer.
 * 
 * Implements all the platform dependent syscall to read and write frame.
 * 
 */
class PhysicalLayer {
	private:
		///< The file descriptor used in read/write syscall
		int file_desc;

		/**
		 * @brief      Get termios baudrate using conversion table
		 *
		 * @param[in]  rawrate  The rawrate
		 *
		 * @return     The baudrate.
		 */
		int get_baudrate(unsigned int rawrate);

		/**
		 * @brief      Read frames
		 *
		 * @param      buff  The buffer
		 * @param[in]  len   The length (multiple of frame size)
		 *
		 * @return     Number of bytes read
		 */
		int read_frames(unsigned char* buff, unsigned int len);

		/**
		 * @brief      Writes frames.
		 *
		 * @param      buff  The buffer
		 * @param[in]  len   The length
		 *
		 * @return     Number of bytes written
		 */
		int write_frames(unsigned char* buff, unsigned int len);

	public:

		/**
		 * @brief      Gets the tick.
		 *
		 * @return     The tick.
		 */
		unsigned long long get_tick();

		/**
		 * @brief      Init serial connection
		 *
		 * @param[in]  device    The device
		 * @param[in]  baudrate  The baudrate
		 *
		 * @return     1 if success, -1 if error
		 */
		int init(const char* device, unsigned int baudrate);

		/**
		 * @brief      Connect and synchronize sender and receiver
		 *
		 * @param[in]  type  The type sender or receiver
		 *
		 * @return     1 if success, 0 otherwise
		 */
		int connect(char type);

		/**
		 * @brief      Send a frame
		 *
		 * @param      f     The frame to send
		 * @param[in]  len   The length (multiple of frame size)
		 *
		 * @return     Number of bytes send (multiple of frame size)
		 */
		int send(frame* f, unsigned int len);

		/**
		 * @brief      Receive a frame
		 *
		 * @param      f     The frame received
		 * @param[in]  len   The length (multiple of frame size)
		 *
		 * @return     Number of bytes received (multiple of frame size)
		 */
		int recv(frame* f, unsigned int len);

		/**
		 * @brief      End serial connection
		 *
		 * @return     0 if success, -1 if error
		 */
		int end();

		/**
		 * @brief      Flush RX serial buffer with a timeout.
		 */
		void flush(unsigned long long timeout);
};


#endif
