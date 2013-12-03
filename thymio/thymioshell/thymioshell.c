/*******************************************
 * Simple Thymio USB Communication 
 * main.c
 *
 * Systems Programming BSc course    (2013)
 * Universita della Svizzera Italiana (USI)
 * 
 * authors: Juxi Leitner <juxi@idsia.ch>, http://Juxi.net/
 *          Alexander Förster <alexander@idsia.ch>
 ********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>	// for open,
#include <unistd.h> // for close
#include <wchar.h>

#ifdef READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "thymio.h"

void disconnect_and_exit(int sig) {
	disconnect(sig);
	printf("\nBye bye by %d!\n",sig);
	exit(EXIT_SUCCESS);
}

int main(int argc,char* argv[]) 
{
	if (argc < 2) {
		fprintf(stderr, "No port specified to read!\nUSAGE: %s robotportinfo\n", argv[0]);
		fprintf(stderr, "Example: %s /dev/cu.usbmodem\n", argv[0]);
		return EXIT_FAILURE;
	}
    
    /* enable CTRL-C callback */
	signal(SIGINT, disconnect_and_exit);

	/* Variables */
	int usb_port;
	char *line = NULL;
	/* structure for returning values from read mehtods */	
	message_t values = {{0, 0, 0}, NULL};

	/* open the USB/serial port like a file */
	if((usb_port = connect(argv[1])) == -1) {
		fprintf(stderr, "Unable to open %s\n", argv[1]);
		return EXIT_FAILURE;
	}

	printf("get description from robot\n");
	send_get_desc_msg(usb_port);

#ifdef READLINE
	while ((line?free(line):0),line = readline ("THYMIO: "))
	{
        if (*line) add_history (line); // non empthy line will be added to the history
        if (!*line) continue; // empty onput will read a new line
#else
	size_t linecap = 0;
	ssize_t linelen;
	while (	printf("THYMIO>"),(linelen = getline(&line, &linecap, stdin)) > 0)
	{
		if (line[linelen-1]=='\n')
		{
			line[linelen-1] = '\0';
			linelen--;
		}
#endif
		char *token, *string;
		string = line;
		token = strsep(&string," \t\n");
		if (token==NULL || strlen(token)==0) continue;
		if (0==strcmp(token,"get"))
		{
			if (string==0 || strlen(string)==0)
			{
				printf("No variable name given!\n");
				printf("USAGE: get <variable>\n");
				continue;
			}
			if(send_get_vars_msg_by_name(usb_port, string, &values)) {
				printf("Name '%s' not found.\n",string);
			} else 	{
				if(values.hdr.len > 0) {
					printf("the %d %s values are: ",(values.hdr.len/2-1),string);
					for (int i = 1;i < values.hdr.len/2; i++) // starting from 1 --> ignore the address value
					{
						printf("[%d]=%u\t",i,((uint16_t*)(values.raw))[i]);
					}
					printf("\n");
				}
			}
		}
		else if (0==strcmp(token,"set"))
		{
			int vals[10]; // we do not expect more than 10 arguments ... oh ... oh...
			char name[256];
			int num = sscanf(string, "%255s %d %d %d %d %d %d %d %d %d %d", name, vals,vals+1,vals+2,vals+3,vals+4,vals+5,vals+6,vals+7,vals+8,vals+9);
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
		else if (0==strcmp(token,"drive"))
		{
			int vals[2]; // we do not expect more than 10 arguments ... oh ... oh...
			int num = sscanf(string, "%d %d", vals,vals+1);
			if (num!=2) {
				printf("Parsing error!\n");
				continue;
			}
			uint16_t hvals[num];
			for (int i=0;i<num;i++)
			{
				hvals[i] = (uint16_t)vals[i];
			}
			int idx; // we cannot use the _by_name, beacuse we will write two variables at the same time
			if ((idx = find_variable_index("motor.left.target"))==-1)
			{
				printf("Ups 'motor.left.target' not found!\n");
				continue;
			};
			uint16_t offset = variables[idx].offset;
			if (send_set_vars_msg(usb_port, offset, hvals, 2))
			{
				printf("Something went wrong while setting the variables.\n");
			};				
		}
		else if (0==strcmp(token,"halt"))
		{
			printf("stopping the robot's motors\n");
			// send_stop_msg(usb_port) will stop the program running on it
			uint16_t hvals[] = {0,0};
			int idx; // we cannot use the _by_name, beacuse we will write two variables at the same time
			if ((idx = find_variable_index("motor.left.target"))==-1)
			{
				printf("Ups 'motor.left.target' not found!\n");
				continue;
			};
			uint16_t offset = variables[idx].offset;
			if (send_set_vars_msg(usb_port, offset, hvals, 2))
			{
				printf("Something went wrong while setting the variables.\n");
			};				
		}
		else if (0==strcmp(token,"reboot"))
		{
			printf("reboot\n");
			send_reboot_msg(usb_port);
		}
		else if (0==strcmp(token,"vars"))
		{
			for (int i=0;i<nvariables;i++)
			{
				printf("%3i: %s [%i]\n",variables[i].offset,variables[i].name,variables[i].num);
			}
		}
		else if (0==strcmp(token,"reset"))
		{
			printf("reset program\n");
			send_reset_msg(usb_port);
		}
		else if (0==strcmp(token,"run"))
		{
			printf("run program\n");
			send_run_msg(usb_port);
		}
		else if (0==strcmp(token,"stop"))
		{
			printf("stop program\n");
			send_stop_msg(usb_port);
		}
		else if (0==strcmp(token,"upload"))
		{
			printf("upload a bytecode program\n");
			if(string == 0 || strlen(string)==0) {
				printf("No file name given!\n");
				printf("USAGE: upload <filename>\n");
				continue;
			}
			
			/* load a program from bytecode */						
			int fd;
			if((fd = open(string, O_RDONLY)) == -1) {
				printf("Could not open file '%s'!\n", string);
				continue;
			}

			uint8_t *raw = (uint8_t*) malloc(ASEBA_MAX_EVENT_ARG_COUNT);
			int num = read(fd, raw, ASEBA_MAX_EVENT_ARG_COUNT); // TODO: check header data and comare with current robot data
			close(fd);

			num -= 2*0x00a;	/* jump over the header in the file */
			num -= 2;		/* do not send the last byte, it's a chksum */

			if(num > 0) send_set_bytecode_msg(usb_port, raw + 2*0x00a, num);

			free(raw);	/* Cleanup! */
		}
		else if (0==strcmp(token,"quit"))
		{
			close(usb_port);
#ifdef READLINE
            if (line) free(line); // Good style to free is here before we exit the progra 
#endif
			printf("Bye!\n");
			return EXIT_SUCCESS;	
		}
		else if (0==strcmp(token,"help"))
		{
			printf("Commands:\n");
			printf("\tdrive <left> <right> : drive the motors with the give speed.\n");
			printf("\thalt  : stop the motors.\n");
			printf("\tget <variable_name> : get (all) the values of a given variable.\n");
			printf("\tset <variable_name> <value1> ... : put (all) the values to a given variable.\n");
			printf("\treboot  : reboot robot.\n");
			printf("\trun : execute the bytecode program.\n");
			printf("\tstop : stop the execution of the bytecode program.\n");	
			printf("\treset : reset the execution of the bytecode program.\n");	
			printf("\tupload <filename> : upload a bytecode command to the robot.\n");	
			printf("\n");
			printf("\tvars  : list all variables and more.\n");
			printf("\tquit  : quit the program.\n");

		}
		else
		{
			printf("What do you mean with '%s'?\n",token);
		};
		fflush(stdout);
	}
	return EXIT_FAILURE;
}
