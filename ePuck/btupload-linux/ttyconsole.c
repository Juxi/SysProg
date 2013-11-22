#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <termios.h>

#define MAX_LINE_LENGTH 100

// #define NON_BLOCKING

int main(int argc,char *argv[])
{
	fprintf(stderr,"************************************\n");
	fprintf(stderr,"*** The ultimate tty console V0 ****\n");
	fprintf(stderr,"************************************\n");
	// open the USB stream
	FILE *fp;
	char *name;
	//char *defname = "/dev/cu.e-puck_1275-COM1-1";
	char *defname = "/dev/tty.usbserial";
	if (argc==1)
	{
		name = defname;
	}
	else if (argc==2)
	{
		name = argv[1];
	}
	else
	{
		name = 0;
		printf("I do not know what I have do do with more than one (here %i) argument.\n",argc);
		return 2;
	}
	
	fp = fopen(name, "w+");
	if (!fp)
	{
		printf("Serial port '%s' cannot be opened.\n",name);
		return 1;
	}
	
	// get the baud rate and the flags
	struct termios term;
	tcgetattr(fileno(fp), &term);
	speed_t sp = cfgetispeed(&term);
	fprintf(stderr,"The current speed is: %i\n", sp);
	sp = cfgetospeed(&term);
	fprintf(stderr,"The current ospeed is: %i\n", sp);
	sp = 115200;
	fprintf(stderr,"setting new baud rate ok: %i\n", cfsetispeed(&term, sp));
	fprintf(stderr,"setting new baud rate ok: %i\n", cfsetospeed(&term, sp));
	
	fprintf(stderr,"The current speed is: %i\n", cfgetispeed(&term));
	fprintf(stderr,"The current ospeed is: %i\n", cfgetospeed(&term));
	
	term.c_cflag |= CLOCAL;
#ifdef NON_BLOCKING
	term.c_cc[VTIME] = 0;
	term.c_cc[VMIN] = 0;
#endif
	tcsetattr(fileno(fp), TCSANOW, &term);

	fprintf(stderr,"Connection established.\n");

	char line[MAX_LINE_LENGTH];

	int pid=fork();
	if (pid == -1)
	{
		printf("Error while forking. errno is %i.\n",errno);
		return 5;
	}
	else if (pid==0) 
	{
		// child
		while(1)
		{
#ifdef NON_BLOCKING			
			size_t nb;
			if ((nb=read(fileno(fp),line,MAX_LINE_LENGTH)) != 0)
			{	
				fwrite(line,1,nb,stdout);
			}
#else
			if ((fgets(line, MAX_LINE_LENGTH, fp)) != NULL)
			{	
				fprintf(stdout,"%s", line);
			}
#endif 
		}		
	}
	else
	{
		// parent
		while(1)
		{
			if ((fgets(line, MAX_LINE_LENGTH, stdin)) != NULL)
			{	
				int num_bytes = fprintf(fp, "%s", line);
				if (num_bytes < 1)
				{
					printf("Error writing to serial port - %s(%d).\n", strerror(errno), errno);
					return 3;
				}
			}
		}
	}
	
	//fprintf(fp,"\r\n");
	
	fclose(fp);
	return 0;
}
