
#include "../include/Protocol.h"


// ------------------------------------------------------------------------- //
// --------------------------- PRIVATE FUNCTIONS --------------------------- //
// ------------------------------------------------------------------------- //

/**
 * Checks if b is between a and c in a circular manner
 */
bool Protocol::between(unsigned char a, unsigned char b, unsigned char c) {
	return ((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a));
}

/**
 * Set max sequence number.
 */
void Protocol::set_max_seqnr(unsigned char max_seqnr) {
	oldest_frame = max_seqnr;
}

/**
 * Return the timedout sequence number.
 */
unsigned char Protocol::get_timedout_seqnr(void) {
	return oldest_frame;
}

/**
 * Set timeout.
 */
void Protocol::set_timeout(unsigned long long timeout) {
	timeout_interval = timeout;
}

/**
 * Allow send_ready events to occur.
 */
void Protocol::enable_protocol(void) {
	status = 1;
}


/**
 * Prevent send_ready events from occuring.
 */
void Protocol::disable_protocol(void) {
	status = 0;
}


/**
 * Set all the member before a new send or receive.
 */
void Protocol::set_up(unsigned char max_seqnr, unsigned long long timeout, int state) {
	if (state == 0)
		disable_protocol();
	else
		enable_protocol();
	offset = 0;
	for (int i = 0; i < WINDOW_SIZE; i++) {
		seqs[i] = MAX_SEQ + 1;
		ack_timer[i] = 0;
		error[i] = false;
	}

	lowest_timer = 0xffffffffffffffff;
	aux_timer = 0;

	set_timeout(timeout);
	set_max_seqnr(max_seqnr);
	inp = &queue[0];						///< where to put the next frame
	outp = &queue[0];						///< where to remove the next frame from
	nframes = 0;
	next_pkt_fetch = 0;						///< seq of next packet from user to fetch
	last_pkt_given = 0;

}



// ----------------------------------------------------------------------------
// CHECKSUM FUNCTION
// ----------------------------------------------------------------------------

/**
 * Compute the checksum to send with the data.
 */
unsigned char Protocol::compute_checksum(unsigned char data[], size_t num_bytes) {
	unsigned char sum = 0;
	for (unsigned int i = 0; i < num_bytes; i++) {
		sum += data[i];
	}
	return ~sum + 1;
}


/**
 * Verify the checksum received with the data.
 */
unsigned char Protocol::verify_checksum(unsigned char data[], size_t num_bytes, unsigned char checksum) {
	unsigned char sum = 0;
	for (unsigned int i = 0; i < num_bytes; i++) {
		sum += data[i];
	}
	sum += checksum;
	return sum;
}



// ----------------------------------------------------------------------------
// PHYSICAL LAYER FUNCTION
// ----------------------------------------------------------------------------

/**
 * Init the protocol, initializing the physical layer with the given parameters.
 */
int Protocol::init(const char* device, int baudrate) {
	return physical_layer.init(device, baudrate);
}


/**
 * Connect and synchronize the sender and receiver
 */
int Protocol::connect(char type) {
	return physical_layer.connect(type);
}


/**
 * Close the physical layer
 */
int Protocol::close() {
	return physical_layer.end();
}

/**
 * Description
 */
void Protocol::flush(unsigned long long timeout) {
	physical_layer.flush(timeout);
}


// ------------------------------------------------------------------------- //
// ---------------------------- PUBLIC FUNCTIONS --------------------------- //
// ------------------------------------------------------------------------- //

// ----------------------------------------------------------------------------
// QUEUE METHOD
// ----------------------------------------------------------------------------

/**
 * See if there is space in the circular buffer queue[].
 * If so try to read as much from the file descriptor as possible.
 * If inp is near the top of queue[], a single call here
 * may read a few frames into the top of queue[] and then some more starting
 * at queue[0]. This is done in two read operations.
 */
void Protocol::enqueue() {

	int reads;
	unsigned int k;
	frame *top;

	///< How many frames can be read consecutively?
	top = (outp <= inp ? &queue[QUEUE_SIZE] : outp);
	///< number of frames that can be read consecutively
	k = top - inp;

	reads = physical_layer.recv(inp, k * sizeof(frame));

	if (reads < 0) {
		if (errno != EAGAIN)
			printf("Error read(): error = %d\n", errno);
		return;
	}

	if (reads % sizeof(frame) != 0) {
		printf("Error read() 1: nreads = %d\n", reads);
	}

	if (reads > 0) {
		nframes = nframes + reads / sizeof(frame);
		inp = inp + reads / sizeof(frame);

		if (inp == &queue[QUEUE_SIZE])
			inp = queue;

		///< are there residual frames to be read?
		if (reads / sizeof(frame) == k) {
			k = outp - inp;

			reads = physical_layer.recv(inp, k * sizeof(frame));

			if (reads < 0) {
				if (errno != EAGAIN)
					printf("Error read(): error = %d\n", errno);
				return;
			}

			if (reads % sizeof(frame) != 0) {
				printf("Error read() 2: nreads = %d\n", reads);
			}

			if (reads > 0) {
				nframes = nframes + reads / sizeof(frame);
				inp = inp + reads / sizeof(frame);
				if (reads / sizeof(frame) == k)
					printf("Queue full");
			}
		}
	}

}

/**
 * This function is called after it has been decided that a frame_arrival
 * event will occur. The earliest frame is removed from queue[] and copied
 * to last_frame.
 * If dequeue() did not remove incoming frames from queue[], they never would be removed.
 * This function determines whether the arrived frame is good
 * or bad (contains a checksum error)
 */
event_type Protocol::dequeue(void) {

	event_type event;

	///< Remove one frame from the queue, copy the first frame in the queue
	last_frame = *outp;
	outp++;
	if (outp == &queue[QUEUE_SIZE])
		outp = queue;
	nframes--;

	// --------------------------------------------------------------------- //
	/**
	 * Generate frames with checksum errors at random
	 */
	/*
	if (last_frame.kind == DATA && error[last_frame.seq] == false) {
		int r = rand() % 10;
		if (r % 3 == 0) {
			for (unsigned int i = 0; i < PKT_SIZE; i++)
				last_frame.info.data[i] += 1;

			error[last_frame.seq] = true;
		}
		//clean the error of the next window element
		if (error[(last_frame.seq + WINDOW_SIZE) % (MAX_SEQ + 1)] != false)
			error[(last_frame.seq + WINDOW_SIZE) % (MAX_SEQ + 1)] = false;
	}
	*/
	// --------------------------------------------------------------------- //

	if (last_frame.kind == DATA) {
		if (verify_checksum(last_frame.info.data, sizeof(last_frame.info.data), last_frame.checksum) != 0)
			event = cksum_err;
		else
			event = frame_arrival;
	} else if (last_frame.kind == ACK || last_frame.kind == NAK) {
		event = frame_arrival;
	} else {
		event = no_event;
	}

	return event;
}



// ----------------------------------------------------------------------------
// EVENT METHOD
// ----------------------------------------------------------------------------

/**
 * Wait_for_event reads the file descriptor to see if any
 * frames are there.  If so, if collects them all in the queue array.
 * Once the file descriptor is empty, it makes a decision about what to do next.
 */
void Protocol::wait_for_event(event_type *event) {

	///< prevents two timeouts at the same tick
	offset = 0;

	while (true) {
		///< go get any newly arrived frames
		enqueue();

		///< Now pick event
		*event = pick_event();

		if (*event == no_event)
			continue;

		return;
	}
}

/**
 * Pick a random event that is now possible for the process.
 * Note that the order in which the tests is made is critical,
 * as it gives priority to some events over others.
 */
event_type Protocol::pick_event(void) {

	if (check_ack_timer() > 0)
		return ack_timeout;

	if (nframes > 0)
		return dequeue();

	if (status)
		return send_ready;

	if (check_timers() >= 0)
		return timeout;

	return no_event;
}



// ----------------------------------------------------------------------------
// TIMERS METHOD
// ----------------------------------------------------------------------------

/**
 * Start a timer for a data frame.
 */
void Protocol::start_timer(unsigned char seqnr) {
	unsigned long long current_time = physical_layer.get_tick();
	ack_timer[seqnr % WINDOW_SIZE] = current_time + timeout_interval + offset;
	offset++;
	///< figure out which timer is now lowest
	recalc_timers();
}

/**
 * Stop a timer for a data frame.
 */
void Protocol::stop_timer(unsigned char seqnr) {
	ack_timer[seqnr % WINDOW_SIZE] = 0;
	///< figure out which timer is now lowest
	recalc_timers();
}

/**
 * Start the auxiliary timer for sending separate acks. The length of the
 * auxiliary timer is arbitrarily set to half the main timer.
 */
void Protocol::start_ack_timer(void) {
	unsigned long long current_time = physical_layer.get_tick();

	aux_timer = current_time + timeout_interval / 2ULL;
	offset++;
}


/**
 * Stop the ack timer.
 */
void Protocol::stop_ack_timer(void) {
	aux_timer = 0;
}


/**
 * Check for possible timeout.  If found, reset the timer.
 * Find the lowest timer.
 * The use of the offset variable guarantees that each successive timer set
 * gets a higher value than the previous one.
 * So when a hit is found, it is the only possibility.
 */
int Protocol::check_timers(void) {
	unsigned long long current_time = physical_layer.get_tick();

	///< See if a timeout event is even possible now.
	if (lowest_timer == 0 || current_time < lowest_timer) 
		return -1;

	for (int i = 0; i < WINDOW_SIZE; i++) {
		if (ack_timer[i] == lowest_timer) {
			///< turn the timer off
			ack_timer[i] = 0;
			///< find new lowest timer
			recalc_timers();
			///< timed out sequence number
			oldest_frame = seqs[i];
			return i;
		}
	}
	printf("Check timers failed at %lld\n", lowest_timer);
	return -1;
}


/**
 * See if the ack timer has expired.
 */
int Protocol::check_ack_timer(void) {
	unsigned long long current_time = physical_layer.get_tick();

	if (aux_timer > 0 && current_time >= aux_timer) {
		aux_timer = 0;
		return 1;
	} else {
		return 0;
	}
}


/**
 * Find the lowest timer.
 */
void Protocol::recalc_timers(void) {

	unsigned long long t = 0xffffffffffffffff;

	for (int i = 0; i < WINDOW_SIZE; i++) {
		if (ack_timer[i] > 0 && ack_timer[i] < t)
			t = ack_timer[i];
	}
	lowest_timer = t;

}



// ----------------------------------------------------------------------------
// LAYER METHOD (from/to application and from/to pyshical)
// ----------------------------------------------------------------------------

/**
 * Fetch a packet from user for transmission on the channel
 * the user data is split into PKT_SIZE bytes data and send in a frame
 */
void Protocol::from_application_layer(unsigned char* data, packet* p) {
	for (unsigned int i = 0; i < sizeof(p->data); i++)
		p->data[i] = data[next_pkt_fetch++];
}


/**
 * Deliver information from an inbound frame to the user layer.
 * the user recv buffer is filled frame by frame.
 */
void Protocol::to_application_layer(unsigned char* data, packet* p) {
	for (unsigned int i = 0; i < sizeof(p->data); i++)
		data[last_pkt_given++] = p->data[i];
}


/**
 * Copy the newly-arrived frame to the user.
 */
void Protocol::from_physical_layer(frame *f) {
	*f = last_frame;
}


/**
 * Pass the frame to the physical layer for writing on serial.
 * The protocol keeps track of sequence numbers using the array seqs[],
 * which records the sequence number of each data frame sent, so on a
 * timeout, knowing the buffer number makes it possible to determine
 * the sequence number.
 */
void Protocol::to_physical_layer(frame *f) {

	int written;

	if (f->kind == DATA)
		seqs[f->seq % WINDOW_SIZE] = f->seq;

	written = physical_layer.send(f, sizeof(frame));

	if (written != sizeof(frame)) {
		if (errno != EAGAIN)
			printf("Error write(): written = %d, error = %d\n", written, errno);
	}

}