/*******************************************
 * Simple Thymio USB Communication 
 * main_test.c
 *
 * Systems Programming BSc course    (2013)
 * Universita della Svizzera Italiana (USI)
 * 
 * author: Juxi Leitner <juxi@idsia.ch>
 *         http://Juxi.net/
 ********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>	// for open, close
#include <unistd.h>

#include "thymio.h"

int main(int argc, char const *argv[]) {
		if (argc < 2) {
		fprintf(stderr, "No port specified to read!\nUSAGE: %s robotportinfo\n", argv[0]);
		fprintf(stderr, "Example: %s /dev/cu.usbmodem\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	/* enable CTRL-C callback */
	signal(SIGINT,disconnect);

	int usb_port;

	/* open the USB/serial port like a file */
	if((usb_port = connect(argv[1])) == -1) {
		fprintf(stderr, "Unable to open %s\n", argv[1]);
		return EXIT_FAILURE;
	}

	message_t msg = {{0, 0, 0}, NULL};

	/* build message structure */
	msg.hdr.typ = (uint16_t) ASEBA_MESSAGE_REBOOT; //ASEBA_MESSAGE_GET_DESCRIPTION; //ASEBA_MESSAGE_REBOOT - 0xA000; // <-- sb
	msg.hdr.len = (uint16_t) 2;					
	msg.raw = (uint8_t *) malloc(msg.hdr.len);
	*msg.raw = (uint16_t) THYMIO_ID;		//destination
	
	/* write some bytes */
	//test_write_message(usb_port, &msg);
	write_message(usb_port, &msg);
	free(msg.raw);		/* Cleanup! */
	
	usleep(SLEEP_MS*10);
	/* read some bytes */
	read_message(usb_port, &msg);


	/* closing the port */
	close(usb_port);

	
	return EXIT_SUCCESS;
}

// void createResetMessage(message_t &msg) {
// 	msg.hdr.typ = 0xa002; // <- eb 
// 	msg.hdr.len = (uint16_t) 2;					
// 	msg.raw = (uint16_t *) malloc(msg.hdr.len);
// 	*msg.raw = (uint16_t) THYMIO_ID;		//destination ;
// }

// void createRebootMessage(message_t &msg) {
// 	msg.hdr.typ = ASEBA_MESSAGE_REBOOT; // <-- sb
// 	msg.hdr.len = (uint16_t) 2;					
// 	msg.raw = (uint16_t *) malloc(msg.hdr.len);
// 	*msg.raw = (uint16_t) THYMIO_ID;		//destination ;
// }

