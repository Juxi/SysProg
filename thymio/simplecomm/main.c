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
#include <string.h>
#include <fcntl.h>	// for open,
#include <unistd.h> // for close
#include <wchar.h>

#include "thymio.h"

#define THYMIO_MOTORS_ADDR 70
#define THYMIO_FLOOR_PROX_ADDR 68

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
	char name[256];
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
				send_get_vars_msg(usb_port, THYMIO_FLOOR_PROX_ADDR, 2, &values);
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

			/* get a variable from the robot */
			case 'g':
				sscanf(line + 2, "%s", name);
				if(send_get_vars_msg_by_name(usb_port, name, &values)) {
					printf("Name '%s' not found.\n",name);
				} else 	{
					if(values.hdr.len > 0) {
						printf("the %d %s values are: ",(values.hdr.len/2-1),name);
						for (int i = 1;i < values.hdr.len/2; i++) // starting from 1 --> ignore the address value
						{
							printf("[%d]=%u\t",i,((uint16_t*)(values.raw))[i]);
						}
						printf("\n");
					}
				}
				break;
			
			/* put a variable into the robot */	
			case 'p': // put
 				{
					int vals[7]; // we expect max 7 arguments 
					int num = sscanf(line+2, "%s %d %d %d %d %d %d %d", name, vals,vals+1,vals+2,vals+3,vals+4,vals+5,vals+6);
					if (num<2) {
						printf("parsing error!");
						continue;
					}
					uint16_t hvals[num-1];
					for (int i=0;i<num-1;i++)
					{
						hvals[i] = (uint16_t)vals[i];
					}
					if(send_set_vars_msg_by_name(usb_port, name, hvals, num-1))
					{
						printf("Name '%s' not found or not %d parameters expected.\n",name,num-1);
					};
				}
				break;


			/* do something to the program */
			case 'b':
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

					/* upload a aseba bytecode program onto the robot*/
					case 'u':
						printf("upload a bytecode program\n");

						if(strlen(line) < 3) {
							printf("no file name given!\n");
							printf("USAGE: bu <filename>\n");
							continue;
						}
						
						/* load a program from bytecode */						
						if(line[strlen(line) - 1] == '\n') line[strlen(line) - 1] = 0; 
						if((h = open(line + 3, O_RDONLY)) == -1) {
							printf("Could not open file '%s'!\n", line + 3);
							continue;
						}

						rp = (uint8_t*) malloc(ASEBA_MAX_EVENT_ARG_COUNT);
						h1 = read(h, rp, ASEBA_MAX_EVENT_ARG_COUNT);
						close(h);

						h1 -= 0x00a;	/* jump over the header in the file */
						h1 -= 1;		/* do not send the last byte, it's a chksum */

						if(h1 > 0) send_set_bytecode_msg(usb_port, rp + 0x00a, h1);

						free(rp);	/* Cleanup! */
						break;


				}
				break;


			/* show help */
			case 'h': 
				printf("Commands:\n");
				printf("\td <left> <right> : drive the motors with the give speed.\n");
				printf("\te  : get motor speed values.\n");
				printf("\ts  : stop the motors.\n");
				printf("\tm  : get floor proximity sensor values left, right.\n");
				printf("\tg <variable_name> : get (all) the values of a given variable.\n");
				printf("\tp <variable_name> <value1> ... : put (all) the values to a given variable.\n");
				printf("\tr  : reset robot.\n");
				printf("\tb<cmd> : relate to the bytecode program in the thymio.\n");
				printf("\tbx : execute the bytecode program.\n");
				printf("\tbs : stop the execution of the bytecode program.\n");	
				printf("\tbu <filename> : upload a bytecode command to the robot.\n");	

				printf("\n");
				printf("\t?  : list all variables and more.\n");
				printf("\tq  : quit the program.\n");
				break;
			/* don't do anything */
			case '\n': printf("\r"); break;

			default: printf("WARNING! Command not found: %c\n", line[0]);
		}
		fflush(stdout);
	}
	
	/* closing the port */
	close(usb_port);

	printf("\nBye bye by q!\n");
	
	return EXIT_SUCCESS;
}
