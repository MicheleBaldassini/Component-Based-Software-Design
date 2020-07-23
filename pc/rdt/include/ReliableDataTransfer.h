
#ifndef RELIABLE_DATA_TRANSFER_H
#define RELIABLE_DATA_TRANSFER_H

#include "Protocol.h"


/**
 * @brief      Class for reliable data transfer.
 * 
 * Implements the high layer for the rdt.
 * It manages 
 */
class ReliableDataTransfer {

	private:
		bool no_nak;						///< no nak has been sent yet
		bool not_expected;
		bool end;							///< To stop send and receive

		unsigned char ack_expected;			///< lower edge of sender's window
		unsigned char next_frame_to_send;	///< upper edge of sender's window + 1
		unsigned char frame_expected;		///< lower edge of receiver's window
		unsigned char too_far;				///< upper edge of receiver's window + 1

		frame r;							///< scratch variable
		packet out_buf[WINDOW_SIZE];		///< buffers for the outbound stream
		packet in_buf[WINDOW_SIZE];			///< buffers for the inbound stream
		bool arrived[WINDOW_SIZE];			///< inbound bit map
		unsigned int nbuffered;				///< how many output buffers currently used

		event_type event;

		Protocol protocol;

		unsigned int nframes;				///< Number of frames to send and receive
		unsigned int last_frame_recv;		///< Counter for frame received
		unsigned int last_frame_send;		///< Counter for frame send

		/**
		 * The functors to rdt implementation function.
		 * Based on user choice it is initizialized
		 * with the corresponding function implementation.
		 */
		void (ReliableDataTransfer::*run)(unsigned char*);

		/**
		 * @brief      Set up the member for a new trasmission.
		 *
		 * @param[in]  len   The length of the user data
		 */
		void set_up(int len);

		/**
		 * @brief      Connect two devices, simple handshaking
		 *
		 * @param[in]  type  The type of connect (sender or receiver)
		 *
		 * @return     1 if success, 0 otherwise
		 */
		int connect(char type);

		/**
		 * @brief      Sends a frame.
		 *
		 * @param[in]  fk              The frame kind
		 * @param[in]  frame_nr        The frame nr
		 * @param[in]  frame_expected  The frame expected
		 * @param      buffer          The data buffer
		 */
		void send_frame(unsigned char fk, unsigned char frame_nr, unsigned char frame_expected, packet buffer[]);

		/**
		 * @brief      Selective repeat implementation
		 *
		 * @param      buff  The buffer to send or receiver
		 */
		void selective_repeat(unsigned char* buff);

		/**
		 * @brief      GO back n implementation
		 *
		 * @param      buff  The buffer to send or receive
		 */
		void go_back_n(unsigned char* buff);

	public:

		/**
		 * @brief      Init the rdt
		 *
		 * @param[in]  device    The device
		 * @param[in]  protocol  The protocol
		 * @param[in]  baudrate  The baudrate
		 *
		 * @return     1 if success, -1 otherwise
		 */
		int init(const char* device, const char* protocol, int baudrate);

		/**
		 * @brief      Send the data
		 *
		 * @param      data     The data
		 * @param[in]  len      The length
		 * @param[in]  timeout  The timeout
		 */
		void send(unsigned char* data, int len, unsigned long timeout);

		/**
		 * @brief      Receive the data
		 *
		 * @param      data     The data
		 * @param[in]  len      The length
		 * @param[in]  timeout  The timeout
		 */
		void recv(unsigned char* data, int len, unsigned long timeout);

		/**
		 * @brief      Close the rdt
		 *
		 * @return     0 if success, -1 otherwise
		 */
		int close();
};

#endif