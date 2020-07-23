
#include <Adafruit_NeoPixel.h>
#include <ReliableDataTransfer.h>


/* Pin on the Arduino connected to the NeoPixels */
#define STRIP_PIN	7

/* Number of NeoPixels attached to the Arduino */
#define NUMPIXELS	144

#define PROTOCOL			"selective repeat" //"go back n"
#define BAUDRATE			115200

// Size of data in bytes that user want to send
#define BUFFER_SIZE			4

/*
 * Parameter 1 = number of pixels in strip
 * Parameter 2 = pin number (most are valid)
 * Parameter 3 = pixel type flags, add together as needed:
 *   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
 *   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
 *   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
 *   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
*/
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, STRIP_PIN, NEO_GRB + NEO_KHZ800);


ReliableDataTransfer rdt = ReliableDataTransfer();

unsigned char buff[BUFFER_SIZE];


void update_strip(unsigned char* data) {
	int k = data[0];
	int r = data[1];
	int g = data[2];
	int b = data[3];

	strip.setPixelColor(k, r, g, b);
	strip.show();
}



void setup() {

	// Init RDT
	rdt.init(PROTOCOL, BAUDRATE);

	// Init Neopixel strip.
	strip.begin();
	strip.show();
}



void loop() {
	// Recv RDT
	rdt.recv(buff, BUFFER_SIZE, 2);

	noInterrupts();
	
	// do stuff
	update_strip(buff);

	interrupts();

	// Send RDT
	rdt.send(buff, BUFFER_SIZE, 1000);
}
