

#include "PhysicalLayer.h"


/**
 * Gets the current time.
 */
unsigned long PhysicalLayer::get_tick() {
	return millis();
}


/**
 * Read bytes from serial.
 * Serial.readBytes() has a timeout, not needed in this case.
 * This read_bytes() is equivalent to set timeout to Serial to 0 after Serial.begin()
 */
int PhysicalLayer::read_bytes(unsigned char* buff, unsigned int len) {
	unsigned int i = 0;
	while (i < len) {
		*buff++ = Serial.read();
		i++;
	}
	return i;
}


/**
 * First we check if there are at least the bytes to compose a frame.
 * If there are read a number of bytes equal or multiple of the size of a frame
 * let's not read anything
 */
int PhysicalLayer::read_frames(unsigned char* buff, unsigned int len) {
	if (len > 0) {
		unsigned int bytes = Serial.available();
		if (bytes >= sizeof(frame)) {
			bytes = bytes - (bytes % sizeof(frame));
			return read_bytes(buff, bytes);
		}
	}
	return 0;
}


/**
 * We wait until there is space in the serial TX buffer to write a frame.
 * When the space is available we write a frame 
 * and return the number of bytes written.
 */
int PhysicalLayer::write_frames(unsigned char* buff, unsigned int len) {
	if (len > 0) {
		//while (Serial.availableForWrite() < len);
		unsigned int i = 0;
		while (i < len) {
			Serial.write(buff[i]);
			i++;
		}
		return i;
	}
	return 0;
}


/**
 * Init the serial connection using HardwareSerial.
 * Set the serial the baudrate given by user and flush
 * the RX serial buffer with a timeout.
 */
int PhysicalLayer::init(unsigned long baudrate) {
	Serial.begin(baudrate);
	flush(1);
	if (Serial)
		return 1;
	return -1;
}


/**
 * To make the connection the sender waits to read a connect byte,
 * whereas the receiver, when ready to receive, will write a connect byte.
 */
int PhysicalLayer::connect(char type) {
	if (type == 's') {
		if (Serial.available() > 0) {
			char c = Serial.read();
			if (c == CONNECT)
				return 1;
		}
		return 0;
	} else if (type == 'r'){
		//if (Serial.availableForWrite() > 0) {
		if (Serial.write(CONNECT) > 0)
			return 1;
		//}
	}
	return 0;
}


/**
 * Send frame
 */
int PhysicalLayer::send(frame* f, unsigned int len) {
	return write_frames((unsigned char*)f, len);
}


/**
 * Receive frame
 */
int PhysicalLayer::recv(frame* f, unsigned int len) {
	return read_frames((unsigned char*)f, len);
}


/**
 * Close the serial connection
 */
int PhysicalLayer::end() {
	Serial.end();
	if (!Serial)
		return 1;
	return -1;
}


/**
 * Flush the RX serial buffer with a timeout.
 * We wait for a timeout, then we read and discard
 * all the available char.
 */
void PhysicalLayer::flush(unsigned long timeout) {
	unsigned char trash[16];
	unsigned long startTime = get_tick();
	while (get_tick() - startTime < timeout);

	read_bytes(trash, sizeof(trash));
}
