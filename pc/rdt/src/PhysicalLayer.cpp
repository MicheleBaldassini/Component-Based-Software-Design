
#include "../include/PhysicalLayer.h"


// ------------------------------------------------------------------------- //
// --------------------------- PRIVATE FUNCTIONS --------------------------- //
// ------------------------------------------------------------------------- //

/**
 * Convert the user baudrate 9600 into a terios baudrate B9600
 * using the conversion table
 */
int PhysicalLayer::get_baudrate(unsigned int rawrate) {

	for (unsigned int i = 0; i < sizeof(conversiontable) / sizeof(conversiontable[0]); i++) {
		if (conversiontable[i].rawrate == rawrate) {
			return conversiontable[i].termiosrate;
		}
	}
	return -1;
}


/**
 * First we check if there are at least the bytes to compose a frame.
 * If there are read a number of bytes equal or multiple of the size of a frame
 * let's not read anything
 */
int PhysicalLayer::read_frames(unsigned char* buff, unsigned int len) {
	if (len > 0) {
		fd_set rfds;
		struct timeval tv;
		int retval;
		unsigned int bytes = 0;

		FD_ZERO(&rfds);
		FD_SET(file_desc, &rfds);

		tv.tv_sec = 0;
		tv.tv_usec = 10;

		retval = select(file_desc + 1, &rfds, NULL, NULL, &tv);

		if (retval < 0)
			return -1;

		if (retval > 0) {
			if (FD_ISSET(file_desc, &rfds)) {
				ioctl(file_desc, FIONREAD, &bytes);
				if (bytes >= sizeof(frame)) {
					bytes = bytes - (bytes % sizeof(frame));
					int nread = read(file_desc, buff, bytes);
					FD_CLR(file_desc, &rfds);
					return nread;
				}
			}
		}
		FD_CLR(file_desc, &rfds);
	}
	return 0;
}


/**
 * We wait until there is space in the file descitpor to write a frame.
 * When the space is available we write a frame 
 * and return the number of bytes written.
 */
int PhysicalLayer::write_frames(unsigned char* buff, unsigned int len) {
	if (len > 0)
		return write(file_desc, buff, len);
	return 0;
}


// ------------------------------------------------------------------------- //
// ---------------------------- PUBLIC FUNCTIONS --------------------------- //
// ------------------------------------------------------------------------- //

/**
 * Gets the current time.
 */
unsigned long long PhysicalLayer::get_tick() {
	struct timeval current_time;
	gettimeofday(&current_time, NULL);

	unsigned long long tick = (current_time.tv_sec * 1000) + (current_time.tv_usec / 1000);
	return tick;
}


/**
 * Init the serial connection using termios struct.
 * Set the serial port parameters to raw mode and the baudrate given by user.
 * Then flush the file descriptor with a timeout.
 * This flush() is needed for one reason:
 * Having tcsetattr(), that flush the RX/TX buffer no timeout,
 * being the serial transmission low, it could be possible
 * that tcsetattr() flush empty buffer and data arrives soon after,
 * leaving the buffer dirty.
 * For this a flush with timeout is needed.
 */
int PhysicalLayer::init(const char* device, unsigned int baudrate) {

	int retry = 0;

	while (retry <= 5) {
		file_desc = open(device, O_RDWR | O_NOCTTY | O_NDELAY);

		if (file_desc != -1)
			break;
		retry++;

		printf("Open() failed: device = %s, attempt number %d of 5\n", device, retry);

		if (retry == 5)
			return -1;

		usleep(1000000);
	}


	termios serialPortSettings;

	if (tcgetattr(file_desc, &serialPortSettings) < 0) {
		perror("tcgetattr() failed: ");
		return -1;
	}

	// ----- CONTROL OPTIONS ----- //
	// Setting parity checking, setting hardware flow control
	serialPortSettings.c_cflag &= ~(CSIZE | PARENB | CSTOPB | CRTSCTS);

	// 8 data bits, enable receiver, local line - do not change "owner" of port
	serialPortSettings.c_cflag |= (CS8 | CREAD | CLOCAL);

	// ----- INPUT OPTIONS ----- //
	// Disable software flow control (ICRNL ignore carriage return on input)
	serialPortSettings.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

	// ----- LINE OPTIONS ----- //
	// Choosing Raw Input (Raw input is unprocessed)
	serialPortSettings.c_lflag &= ~(ECHO | ECHOE | ECHONL | ICANON | ISIG | IEXTEN);

	// ----- OUTPUT OPTIONS ----- //
	// Choosing Raw Output
	serialPortSettings.c_oflag &= ~OPOST;

	// ----- CONTROL CHARACTERS ----- //
	// Minimum number of characters to read
	serialPortSettings.c_cc[VMIN]  = 0;
	// Time to wait for data (tenths of seconds)
	serialPortSettings.c_cc[VTIME] = 10;

	// Setting the Baud Rate
	int baud = get_baudrate(baudrate);

	if (cfsetispeed(&serialPortSettings, baud) < 0 || cfsetospeed(&serialPortSettings, baud) < 0) {
		perror("cfsetXspeed() failed: ");
		return -1;
	}

	// Flush input and output buffers and make the change
	if (tcsetattr(file_desc, TCSAFLUSH, &serialPortSettings) < 0) {
		perror("tcsetattr() failed: ");
		return -1;
	}

	flush(1);

	// Arduino boot time: 1.7 seconds
	usleep(1700000);

	return file_desc;

}


/**
 * To make the connection the sender waits to read a connect byte,
 * whereas the receiver, when ready to receive, will write a connect byte.
 */
int PhysicalLayer::connect(char type) {
	if (type == 's') {
		unsigned int bytes = 0;
		ioctl(file_desc, FIONREAD, &bytes);
		if (bytes > 0) {
			int nread = -1;
			char c = 0;
			nread = read(file_desc, &c, 1);
			if (nread > 0 && c == CONNECT) {
				return 1;
			}
		}
		return 0;
	} else if (type == 'r') {
		int nwrite = -1;
		char c = CONNECT;
		nwrite = write(file_desc, &c, 1);
		if (nwrite > 0)
			return 1;
		return 0;
	}
	return 0;
}


/**
 * Send frame
 */
int PhysicalLayer::send(frame *f, unsigned int len) {
	return write_frames((unsigned char*)f, len);
}


/**
 * Receive frame
 */
int PhysicalLayer::recv(frame *f, unsigned int len) {
	return read_frames((unsigned char*)f, len);
}


/**
 * Close the serial connection
 */
int PhysicalLayer::end() {
	return close(file_desc);
}


/**
 * Flush the file descriptor buffer with a timeout.
 * We wait for a timeout, then we read and discard
 * all the available char.
 */
void PhysicalLayer::flush(unsigned long long timeout) {
	unsigned char trash[16];
	unsigned long long startTime = get_tick();
	while (get_tick() - startTime < timeout);

	read(file_desc, trash, sizeof(trash));
}

/*
	fd_set rfds;
	struct timeval tv;
	int retval;
	unsigned int bytes = 0;

	FD_ZERO(&rfds);
	FD_SET(file_desc, &rfds);

	tv.tv_sec = 0;
	tv.tv_usec = 100;

	retval = select(file_desc + 1, &rfds, NULL, NULL, &tv);

	if (retval < 0)
		return;

	if (retval > 0) {
		if (FD_ISSET(file_desc, &rfds)) {
			ioctl(file_desc, FIONREAD, &bytes);
			if (bytes > 0) {
				unsigned char trash[16];
				read(file_desc, trash, bytes);
			}
		}
	}
	FD_CLR(file_desc, &rfds);
}
*/

