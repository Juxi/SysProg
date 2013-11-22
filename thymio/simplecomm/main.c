/*******************************************
 * Simple Thymio USB Communication 
 * main.c
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

#include "thymio.h"

#define PROMPT "> "


//int create_message(message_t &msg, int type);
void create_message(message_t *msg, uint16_t type, void* data, uint16_t len);
int send_reboot_msg(int port);

int main(int argc, char const *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "No port specified to read!\nUSAGE: %s robotportinfo\n", argv[0]);
		fprintf(stderr, "Example: %s /dev/cu.usbmodem\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* Variables */
	int usb_port;
	int c, running = 1;

	/* open the USB/serial port like a file */
	if((usb_port = connect(argv[1])) == -1) {
		fprintf(stderr, "Unable to open %s\n", argv[1]);
		return EXIT_FAILURE;
	}

	/* main program control loop */
	while ( running ) {
		printf(PROMPT);
		c = getchar();

		switch(c) {
			/* we're done, exit the control lopp */
			case EOF:
			case 'q':
				running = 0;
				break;

			/* reboot the robot */
			case 'r':
				send_reboot_msg(usb_port);
				break;
			
			/* drive the robot */
			case 'd':

				break;

			default: printf("WARNING! Command not found: %c\n", c);
		}
		fflush(stdout);
	}
	

	/* closing the port */
	close(usb_port);

	
	return EXIT_SUCCESS;
}

int send_reboot_msg(int port) {
	message_t msg = {{0, 0, 0}, NULL};

	/* build message structure */
	uint16_t data = THYMIO_ID;
	create_message(&msg, ASEBA_MESSAGE_REBOOT, &data, 2);

	// msg.hdr.typ = (uint16_t) ASEBA_MESSAGE_REBOOT; //ASEBA_MESSAGE_GET_DESCRIPTION; //ASEBA_MESSAGE_REBOOT - 0xA000; // <-- sb
	// msg.hdr.len = (uint16_t) 2;					
	// msg.raw = (uint8_t *) malloc(msg.hdr.len);
	// *msg.raw = (uint16_t) THYMIO_ID;		//destination
	
	/* write some bytes */
	//test_write_message(usb_port, &msg);
	write_message(port, &msg);
	free(msg.raw);		/* Cleanup! */
	
	usleep(SLEEP_MS*10);
	/* read some bytes */
	printf("%d is read len\n", read_message(port, &msg));

	return 0;
}

void create_message(message_t *msg, uint16_t type, void *data, uint16_t len) {
	/* build message structure */
	msg->hdr.typ = (uint16_t) type;
	msg->hdr.len = (uint16_t) 2;				
	msg->raw = (uint8_t *) malloc(msg->hdr.len);
	// msg->raw[0] = ((uint8_t*) data)[0];		//destination
		msg->raw[i] = ((uint8_t*) data)[i];		//destination
}

// void create_message(message_t *msg, uint16_t type, void *data, uint16_t len) {
// 	/* build message structure */
// 	msg->hdr.typ = (uint16_t) type; //ASEBA_MESSAGE_GET_DESCRIPTION; //ASEBA_MESSAGE_REBOOT - 0xA000; // <-- sb
// 	msg->hdr.len = (uint16_t) 2;				
// 	msg->raw = (uint8_t *) malloc(msg->hdr.len);
// 	msg->raw[0] = ((uint8_t*) data)[0];		//destination
// }

