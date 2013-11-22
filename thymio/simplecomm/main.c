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
#include <wchar.h>

#include "thymio.h"

#define PROMPT "> "

int mygetline(char s[], int n) {
	int i;
	for (i = 0; i < n; ++i) 
	{
		s[i] = getchar();
		if (s[i] == EOF || s[i] == '\n')
			break;
	}
	if (s[i]==EOF && ferror(stdin))
		return -1;
	if (s[i]==EOF && i==0)
		return 0;
	if (s[i]!='\n')
		return -1;
	s[i] = '\0';
	return 1;
}

//int create_message(message_t &msg, int type);
void create_message(message_t *msg, uint16_t type, void* data, uint16_t len);
int send_reboot_msg(int port);
int send_stop_msg(int port);
int send_get_desc_msg(int port);
int send_get_vars_msg(int port);
int send_set_vars_msg(int port);


/* * * * * * * * * * * * * * * * * * * * *
 * main function
*/
int main(int argc, char const *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "No port specified to read!\nUSAGE: %s robotportinfo\n", argv[0]);
		fprintf(stderr, "Example: %s /dev/cu.usbmodem\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* Variables */
	int usb_port;
	int running = 1;
	size_t linecap = 0;
	char *line = NULL;

	/* open the USB/serial port like a file */
	if((usb_port = connect(argv[1])) == -1) {
		fprintf(stderr, "Unable to open %s\n", argv[1]);
		return EXIT_FAILURE;
	}


	/* main program control loop */
	while ( running ) {
		printf(PROMPT);

		getline(&line, &linecap, stdin);
		switch(line[0]) {
			/* we're done, exit the control lopp */
			case EOF:
			case 'q':
				printf("quit\n");
				running = 0;
				break;

			/* get robot info*/
			case '?':
				printf("get desc\n");
				send_get_desc_msg(usb_port);
				break;

			/* reboot the robot */
			case 'r':
				printf("reboot\n");
				send_reboot_msg(usb_port);
				break;
			
			/* get motor speed left, right */
			case 'e': case 'E':
				printf("the motor speeds are:\t");
				send_get_vars_msg(usb_port);

				printf("left=%d\tright=%d\n", 0, 0);
				break;

			/* drive the robot */
			case 'd':

				break;

			/* stop the robot */
			case 's':
				send_stop_msg(usb_port);
				break;

			/* don't do anything */
			case '\n': printf("\r"); break;

			default: printf("WARNING! Command not found: %c\n", line[0]);
		}
		fflush(stdout);
	}
	
	/* closing the port */
	close(usb_port);

	
	return EXIT_SUCCESS;
}

int send_reboot_msg(int port) {
	message_t msg = {{0, 0, 0}, NULL};

	/* clear input buffer */

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
	printf("received %d bytes\n", read_message(port, &msg));

	return 0;
}


int send_stop_msg(int port) {
	message_t msg = {{0, 0, 0}, NULL};

	/* build message structure */
	uint16_t data = THYMIO_ID;
	create_message(&msg, ASEBA_MESSAGE_STOP, &data, 2);
	write_message(port, &msg);
	free(msg.raw);		/* Cleanup! */
	
	usleep(SLEEP_MS*10);
	/* read some bytes */
	printf("received %d bytes\n", read_message(port, &msg));

	uint16_t value;
	parse_from_raw(msg.raw, &value);
	printf("value of #%d is: %d\n", 0, value);
	parse_from_raw(msg.raw + 2, &value);
	printf("value of #%d is: %d\n", 1, value);


	return 0;
}


int send_get_desc_msg(int port) {
	message_t msg = {{0, 0, 0}, NULL};
	uint16_t n_named_vars, n_local_events, n_native_funcs;
	char *s;


	/* build message structure */
	uint16_t data = (uint16_t)ASEBA_PROTOCOL_VERSION;
	create_message(&msg, ASEBA_MESSAGE_GET_DESCRIPTION, &data, 2);
	write_message(port, &msg);
	free(msg.raw);		/* Cleanup! */
	
	usleep(SLEEP_MS*10);
	/* read some bytes */
	if(read_message(port, &msg) == 0) {
		printf("Warning! We did not receive a reply!\n");
		return -1;
	}
	parse_desc_reply(&msg,
		&n_named_vars, &n_local_events, &n_native_funcs);

	
	/* next messages contain the named variables */
	printf("reading named variables... ");
	read_named_variables(port, n_named_vars);
	printf("done!\n");

	/* next messages contain the local events */
	printf("reading local events...");
	read_local_events(port, n_local_events);
	printf("done!\n");

	/* next messages contain the local events */
	printf("reading native functions...");
	read_native_functions(port, n_native_funcs);
	printf("done!\n");


	printf("Done parsing description messages!\n");
	return 0;
}

int send_get_vars_msg(int port) {
	message_t msg = {{0, 0, 0}, NULL};
	uint16_t idx = 0, n_values = 1;
	uint16_t value = -1;

	// L"motor.left.speed"
	/* build message structure */
	uint16_t data[3];

	for(idx = 516; idx < 800; idx++) {
			data[0] = (uint16_t) THYMIO_ID;
			data[1] = (uint16_t) idx;
			data[1] = (uint16_t) n_values;
			create_message(&msg, ASEBA_MESSAGE_GET_VARIABLES, &data, sizeof(data));
			write_message(port, &msg);
			free(msg.raw);		/* Cleanup! */
			
			usleep(SLEEP_MS*10);
			/* read some bytes */
			if(read_message(port, &msg) == 0) {
				printf("Warning! We did not receive a reply!\n");
				return -1;
			}
			printf("received %d bytes\n", msg.hdr.len + 6);
	
			// do something with the message!!
			parse_from_raw(msg.raw, &value);
			printf("value of #%d is: %d\n", idx, value);
			//free(msg.raw);
	}
	printf("Done parsing variables!\n");
	return 0;
}

