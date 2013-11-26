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
#include <fcntl.h>	// for open,
#include <unistd.h> // for close
#include <wchar.h>

#include "thymio.h"

#define THYMIO_MOTORS_ADDR 70

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


/* * * * * * * * * * * * * * * * * * * * *
 * main function
*/
int main(int argc, char const *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "No port specified to read!\nUSAGE: %s robotportinfo\n", argv[0]);
		fprintf(stderr, "Example: %s /dev/cu.usbmodem\n", argv[0]);
		return EXIT_FAILURE;
	}
    
    /* enable CTRL-C callback */
	signal(SIGINT,disconnect);

	/* Variables */
	int usb_port;
	int running = 1;
	size_t linecap = 0;
	char *line = NULL;
	int h, h1;
	uint16_t val = 0, hvals[2];
	uint8_t *rp;	/* temporary raw pointer */
	/* structure for returning values from read mehtods */	
	message_t values = {{0, 0, 0}, NULL};

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
				printf("get description from robot\n");
				send_get_desc_msg(usb_port);
				break;

			/* reboot the robot */
			case 'r':
				printf("reboot\n");
				send_reboot_msg(usb_port);
				break;
			
			/* get floor proximity sensor values left, right */
			case 'm': case 'M':
				send_get_vars_msg(usb_port, 68, 2, &values);
				if(values.hdr.len == 6) {
					rp = values.raw;
					
					/* read the address */
					read_from_raw(&rp, &val);
					
					printf("the floor proximity readouts are: ");
					/* read the values */
					read_from_raw(&rp, &val); printf("left =%3d\t", val);
					read_from_raw(&rp, &val); printf("right=%3d\n", val);
				}
				break;


			/* get motor speed left, right */
			case 'e': case 'E':
				sscanf(line+2, "%d", &h);
				send_get_vars_msg(usb_port, THYMIO_MOTORS_ADDR, 2, &values);
				if(values.hdr.len == 6) {
					rp = values.raw;
					
					/* read the address */
					read_from_raw(&rp, &val);
					
					printf("the motor speeds are: ");
					/* read the values */
					read_from_raw(&rp, &val); printf("left =%3d\t", val);
					read_from_raw(&rp, &val); printf("right=%3d\n", val);
				}
				break;


			/* drive the robot */
			case 'd':
				if(2 == sscanf(line+2, "%d,%d", &h, &h1)) {
				hvals[0] = (uint16_t) h;
				hvals[1] = (uint16_t) h1;
				send_set_vars_msg(usb_port, THYMIO_MOTORS_ADDR, hvals, 2);
				}else printf("parsing error!");
				break;

			/* stop the robot's motors */
			case 's':
				printf("stopping the robot's motors\n");
				// send_stop_msg(usb_port) will stop the program running on it
				*hvals = *(hvals + 1) = 0;
				send_set_vars_msg(usb_port, THYMIO_MOTORS_ADDR, hvals, 2);
				break;




			/* do something to the program */
			case 'p':
				switch(line[1]) {
					/*execute / run the aseba bytecode on the robot*/
					case 'x':
						printf("run program\n");
						send_run_msg(usb_port);
						break;
					 
					/* stop the aseba bytecode on the robot*/
					case 's':
						printf("stop program\n");
						send_stop_msg(usb_port);
						break;

				}
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
