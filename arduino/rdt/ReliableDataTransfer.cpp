
#include "ReliableDataTransfer.h"

// ------------------------------------------------------------------------- //
// PRIVATE FUNCTIONS
// ------------------------------------------------------------------------- //

void ReliableDataTransfer::set_up(int len) {
	no_nak = true;
	this->end = false;
	not_expected = false;

	ack_expected = 0;			// next ack expected on the inbound stream
	next_frame_to_send = 0;		// number of next outgoing frame
	frame_expected = 0;			// frame number expected
	too_far = WINDOW_SIZE;		//receiver's upper window + 1

	nbuffered = 0;				// initially no packets are buffered

	for (int i = 0; i < WINDOW_SIZE; i++)
		arrived[i] = false;

	if (len < PKT_SIZE)
		nframes = 1;
	else
		nframes = len / PKT_SIZE;

	last_frame_recv = 0;
	last_frame_send = 0;
}



int ReliableDataTransfer::connect(char type) {
	return protocol.connect(type);
}



/**
 * Construct and send a data, ack, or nak frame.
 */
void ReliableDataTransfer::send_frame(unsigned char fk, unsigned char frame_nr, unsigned char frame_expected, packet buffer[]) {
	frame f;

	///< kind == data, ack, or nak
	f.kind = fk;

	f.seq = frame_nr;
	f.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);

	f.info = buffer[frame_nr % WINDOW_SIZE];
	f.checksum = protocol.compute_checksum(f.info.data, sizeof(f.info.data));

	///< one nak per frame, please
	if (fk == NAK)
		no_nak = false;
	///< transmit the frame
	protocol.to_physical_layer(&f);

	if (fk == DATA)
		protocol.start_timer(frame_nr);

	///< no need for separate ack frame
	protocol.stop_ack_timer();

}


/**
 * Selective repeat implementations to allow a relieable data transfer
 * using a sliding window protocol, having two windows (one for send and one for receive).
 * 
 */
void ReliableDataTransfer::selective_repeat(unsigned char* buff) {

	protocol.wait_for_event(&event);

	switch (event) {

		///< accept, save, and transmit a new frame
		case send_ready:
			///< expand the window
			nbuffered = nbuffered + 1;
			///< fecth data from user (divide user data in 4 bytes frame)
			protocol.from_application_layer(buff, &out_buf[next_frame_to_send % WINDOW_SIZE]);
			///< transmit the frame
			send_frame(DATA, next_frame_to_send, frame_expected, out_buf);
			///< advance upper window edge
			inc(next_frame_to_send);

			last_frame_send = last_frame_send + 1;
			break;

		///< a control frame has arrived (ack or nak)
		case frame_arrival:
			///< fetch incoming frame from physical layer (serial)
			protocol.from_physical_layer(&r);

			if (r.kind == DATA) {

				///< An undamaged frame has arrived
				if (r.seq != frame_expected) {
					if (no_nak) {
						not_expected = true;
						send_frame(NAK, 0, frame_expected, out_buf);
					}
				} else {
					not_expected = false;
					protocol.start_ack_timer();
				}

				///< Frames may be accepted in any order
				if (protocol.between(frame_expected, r.seq, too_far) && (arrived[r.seq % WINDOW_SIZE] == false)) {
					///< mark buffer as full
					arrived[r.seq % WINDOW_SIZE] = true;
					///< insert data into buffer
					in_buf[r.seq % WINDOW_SIZE] = r.info;

					while (arrived[frame_expected % WINDOW_SIZE]) {
						///< Pass frames and advance window. 
						protocol.to_application_layer(buff, &in_buf[frame_expected % WINDOW_SIZE]);

						no_nak = true;

						arrived[frame_expected % WINDOW_SIZE] = false;
						///< advance lower edge of receiver's window
						inc(frame_expected);
						///< advance upper edge of receiver's window
						inc(too_far);
						///< count total received data frame
						last_frame_recv = last_frame_recv + 1;
						///< to see if a separate ack is needed
						protocol.start_ack_timer();
					}
				}

				//if (not_expected && last_frame_recv == nframes)
					//end = true;

			}

			if ((r.kind == NAK) && protocol.between(ack_expected, (r.ack + 1) % (MAX_SEQ + 1), next_frame_to_send))
				send_frame(DATA, (r.ack + 1) % (MAX_SEQ + 1), frame_expected, out_buf);

			while (protocol.between(ack_expected, r.ack, next_frame_to_send)) {
				///< handle piggybacked ack
				nbuffered = nbuffered - 1;
				///< frame arrived intact
				protocol.stop_timer(ack_expected);
				///< advance lower edge of sender's window
				inc(ack_expected);
				///< count total received ack frame
				last_frame_recv = last_frame_recv + 1;
			}

			if (r.kind == DATA && not_expected) {
				if (last_frame_recv == nframes)
					end = true;
			} else if (r.kind == ACK || r.kind == NAK) {
				if (last_frame_recv == nframes)
					end = true;
			}

			break;

		///< we timed out
		case timeout:
			send_frame(DATA, protocol.get_timedout_seqnr(), frame_expected, out_buf);
			break;

		///< damaged frame
		case cksum_err:
			if (no_nak) {
				send_frame(NAK, 0, frame_expected, out_buf);
				if (last_frame_recv == nframes)
					end = true;
			}
			break;

		///< ack timer expired; send ack
		case ack_timeout:
			send_frame(ACK, 0, frame_expected, out_buf);
			if (last_frame_recv == nframes)
				end = true;
			break;

		///< no event
		default:
			break;
	}
}


/**
 * Go Back N implementations. Only one window, sender side is needed
 * to realize a reliable data transfer.
 */
void ReliableDataTransfer::go_back_n(unsigned char* buff) { 

	protocol.wait_for_event(&event);

	switch(event) {

		case send_ready:
			///< expand the sender's window
			nbuffered = nbuffered + 1;
			///< fecth data from user (divide user data in 4 bytes frame)
			protocol.from_application_layer(buff, &out_buf[next_frame_to_send % WINDOW_SIZE]);
			///< transmit the frame
			send_frame(DATA, next_frame_to_send, frame_expected, out_buf);
			///< advance sender's upper window edge
			inc(next_frame_to_send);

			last_frame_send = last_frame_send + 1;
			break;

		///< a data or control frame has arrived
		case frame_arrival:
			///< get incoming frame from physical layer
			protocol.from_physical_layer(&r);

			///< Frames are accepted only in order
			if (r.seq == frame_expected) {
				///< insert data into buffer
				in_buf[r.seq % WINDOW_SIZE] = r.info;
				///< Pass frames and advance window.
				protocol.to_application_layer(buff, &in_buf[frame_expected % WINDOW_SIZE]);
				///< advance lower edge of receiver's window
				inc(frame_expected);

				last_frame_recv = last_frame_recv + 1;

				if ((r.seq % WINDOW_SIZE) == WINDOW_SIZE - 1)
					send_frame(ACK, 0, frame_expected, out_buf);
			}

			///< Ack n implies n - 1, n - 2, etc.  Check for this.
			while (protocol.between(ack_expected, r.ack, next_frame_to_send)) {
				///< Handle piggybacked ack (one frame fewer buffered)
				nbuffered = nbuffered - 1;
				///< frame arrived intact; stop timer
				protocol.stop_timer(ack_expected);
				///< contract sender's window
				inc(ack_expected);

				last_frame_recv = last_frame_recv + 1;
			}

			if (last_frame_recv == nframes)
				end = true;
			break;

		case cksum_err:
			break;

		///< trouble; retransmit all outstanding frames
		case timeout:
			///< start retransmitting here
			next_frame_to_send = ack_expected;
			for (unsigned int i = 0; i < nbuffered; i++) {
				///< resend 1 frame
				send_frame(DATA, next_frame_to_send, frame_expected, out_buf);
				///< prepare to send the next one
				inc(next_frame_to_send);
			}
			break;

		default:
			break;
	}
}


// ------------------------------------------------------------------------- //
// PUBLIC FUNCTIONS
// ------------------------------------------------------------------------- //

int ReliableDataTransfer::init(const char* prot, unsigned long baudrate) {
	if (strcmp(prot, "selective repeat") == 0)
		this->run = &ReliableDataTransfer::selective_repeat;
	if (strcmp(prot, "go back n") == 0)
		this->run = &ReliableDataTransfer::go_back_n;
	return protocol.init(baudrate);
}


void ReliableDataTransfer::send(unsigned char* buff, int len, unsigned long timeout) {

	this->set_up(len);
	protocol.set_up((MAX_SEQ + 1), timeout, 1);

	while (connect('s') < 1);

	while (end == false) {
		(this->*run)(buff);

		if (nbuffered < WINDOW_SIZE && last_frame_send < nframes)
			protocol.enable_protocol();
		else
			protocol.disable_protocol();
	}
}


void ReliableDataTransfer::recv(unsigned char* buff, int len, unsigned long timeout) {

	this->set_up(len);
	protocol.set_up((MAX_SEQ + 1), timeout, 0);

	while (connect('r') < 1);

	while (end == false) {
		(this->*run)(buff);
	}

	//protocol.flush();
}



int ReliableDataTransfer::close() {
	return protocol.close();
}
