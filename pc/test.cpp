

// COMUNICATION WITH ARDUINO

#include "rdt/include/ReliableDataTransfer.h"

#define INVALID_SERIAL -1
#define NUM_PIXELS 144


// For reliable data transfer init
// User can choose:
// the device to open, the protocol to use to ensure reliable connection,
// some device parameters for the connection (baudrate for serial)
#define DEVICE				"/dev/ttyACM0"
#define PROTOCOL			"selective repeat" //"go back n"
#define BAUDRATE			115200


// Size of data in bytes that user want to send
#define BUFFER_SIZE			4


void sender(ReliableDataTransfer rdt, unsigned char* buffer, unsigned char r, unsigned char g, unsigned char b) {
	struct timeval start_time, end_time;
	unsigned long long start_tick, end_tick;

	// Turn on led
	for (int i = 0; i < NUM_PIXELS; i++) {
		buffer[0] = i;
		buffer[1] = r;
		buffer[2] = g;
		buffer[3] = b;

		// Send the command and receive ack
		gettimeofday(&start_time, NULL);
		start_tick = (start_time.tv_sec * 1000) + (start_time.tv_usec / 1000);

		printf("|X| PC ----> ARDUINO |X|\n");
		rdt.send(buffer, BUFFER_SIZE, 1000);

		// Receive the command and send ack
		printf("|X| ARDUINO ----> PC |X|\n");
		rdt.recv(buffer, BUFFER_SIZE, 2);

		gettimeofday(&end_time, NULL);
		end_tick = (end_time.tv_sec * 1000) + (end_time.tv_usec / 1000);

		printf("Elapsed time ===> %lld ms\n\n", end_tick - start_tick);

	}

	// Turn off led
	for (int i = NUM_PIXELS - 1; i >= 0; i--) {
		buffer[0] = i;
		buffer[1] = 0;
		buffer[2] = 0;
		buffer[3] = 0;

		// Send the command and receive ack
		gettimeofday(&start_time, NULL);
		start_tick = (start_time.tv_sec * 1000) + (start_time.tv_usec / 1000);

		printf("|X| PC ----> ARDUINO |X|\n");
		rdt.send(buffer, BUFFER_SIZE, 1000);

		// Receive the command and send ack
		printf("|X| ARDUINO ----> PC |X|\n");
		rdt.recv(buffer, BUFFER_SIZE, 2);

		gettimeofday(&end_time, NULL);
		end_tick = (end_time.tv_sec * 1000) + (end_time.tv_usec / 1000);

		printf("Elapsed time ===> %lld ms\n\n", end_tick - start_tick);

	}
}



int main() {

	ReliableDataTransfer rdt;
	unsigned char buffer[BUFFER_SIZE];
	
	if (rdt.init(DEVICE, PROTOCOL, BAUDRATE) > 0) {
		printf("%s\n", "Open rdt");

		sender(rdt, buffer, 10, 0, 0);
		sender(rdt, buffer, 0, 10, 0);
		sender(rdt, buffer, 0, 0, 10);
		sender(rdt, buffer, 10, 0, 0);
		sender(rdt, buffer, 0, 10, 0);
		sender(rdt, buffer, 0, 0, 10);
		sender(rdt, buffer, 10, 0, 0);
		sender(rdt, buffer, 0, 10, 0);
		sender(rdt, buffer, 0, 0, 10);
		sender(rdt, buffer, 10, 0, 0);
		sender(rdt, buffer, 0, 10, 0);
		sender(rdt, buffer, 0, 0, 10);

		if (rdt.close() == 0)
			printf("%s\n", "Close rdt");
		else
			printf("%s\n", "Error in close rdt");

	} else {
		printf("%s\n", "Error in open rdt");
	}

	return 0;
}

