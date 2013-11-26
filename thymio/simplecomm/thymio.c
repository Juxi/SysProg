/*******************************************
 * Simple Thymio USB Communication 
 * thymio.h
 *
 * Systems Programming BSc course    (2013)
 * Universita della Svizzera Italiana (USI)
 * 
 * author: Juxi Leitner <juxi@idsia.ch>
 *         http://Juxi.net/
 ********************************************/

#include "thymio.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>	// for open, close

#include <unistd.h>	// for read and usleep
#include <signal.h>
#include <termios.h> // for settings
#include <string.h> // for memset

/* Global variable  */
static int intFH; /* keep a copy of the port handle for the interrupt signal */


int connect(const char *port_name) {
	int port;

	if((port = open(port_name, O_RDWR)) < 0)
		return -1;

	//int lockRes = flock(port, LOCK_EX|LOCK_NB);

	intFH = port;	/* in case the interrupt handler needs to close the port */
	printf("Connected to %s ... ", port_name);

	/* Configure the port */
	/* ser:port=1;baud=115200;stop=1;parity=none;fc=none;bits=8 */
	if(configure(port) < 0)
		return -1;
	printf("and configured!\n");
	usleep(SLEEP_MS);

	// what does aseba do!?
	// it asks for the description to set the robot_node_id
	return port;
}

int configure(int port) {
	/* ser:port=1;baud=115200;stop=1;parity=none;fc=none;bits=8 */
	struct termios settings, oldtio;

	// save old serial port state and clear new one
    tcgetattr(port, &oldtio);
    memset(&settings, 0, sizeof(settings));
                        
	settings.c_cflag |= CLOCAL;              // ignore modem control lines.
	settings.c_cflag |= CREAD;               // enable receiver.
	settings.c_cflag |= CS8; 
	//settings.c_cflag |= PARENB;              // enable parity generation on output and parity checking for input.
                        
	if (cfsetspeed(&settings,115200) != 0) {
		perror("could not set baudrate/speed");
		return -1;
	}
	// on non mac do it like this: newtio.c_cflag |= B115200; break;
                        
	settings.c_iflag = IGNPAR;               // ignore parity on input
	settings.c_oflag = 0;
	settings.c_lflag = 0;
	settings.c_cc[VTIME] = 0;                // block forever if no byte
	settings.c_cc[VMIN] = 1;                 // one byte is sufficient to return
                        
	/* set attributes */
	if ((tcflush(port, TCIOFLUSH) < 0) || (tcsetattr(port, TCSANOW, &settings) < 0)) {
		perror("Cannot setup serial port. The requested baud rate might not be supported.");
		return -1;
	}

	return 0;
}

/* create a message structure for the aseba communication */
void create_message(message_t *msg, uint16_t type, void *data, uint16_t len) {
	/* build message structure */
	msg->hdr.typ = (uint16_t) type;
	msg->hdr.len = (uint16_t) len;				
	msg->raw = (uint8_t *) malloc(msg->hdr.len);
	// msg->raw[0] = ((uint8_t*) data)[0];		//destination
	for(int i=0;i < msg->hdr.len; i++)
		msg->raw[i] = ((uint8_t*) data)[i];		//destination
}



/* warning this destroys the message! */
int write_message(int port, message_t *msg) {
	// TODO:
	if (msg->hdr.len > ASEBA_MAX_EVENT_ARG_SIZE) {	// 258 * 2 ?! from consts.h
		fprintf(stderr, "fatal error: message size exceed maximum packet size.\n");
		fprintf(stderr, "message payload size: %d\n", msg->hdr.len);
		fprintf(stderr, "maximum packet payload size: %d\n", ASEBA_MAX_EVENT_ARG_SIZE);
		return -1;
	}
	int written_bytes = 0;

	// print_mesage_header(&msg);
	
	/* write the message header (len, src, type) */
	written_bytes = write(port, &msg->hdr.len, 2);
	written_bytes += write(port, &msg->hdr.src, 2);
	written_bytes += write(port, &msg->hdr.typ, 2);

	written_bytes += write(port, msg->raw, msg->hdr.len);

	if(written_bytes < 6 + msg->hdr.len) perror("ERROR writing message");	

	fsync(port);
	printf("wrote %d bytes\n", written_bytes);
	return written_bytes;


// maybe look into that
 // while (left)
 //                        {
 //                                ssize_t len = ::write(fd, ptr, left);
                                
 //                                if (len < 0)
 //                                {
 //                                        fail(DashelException::IOError, errno, "File write I/O error.");
 //                                }
 //                                else if (len == 0)
 //                                {
 //                                        fail(DashelException::ConnectionLost, 0, "File full.");
 //                                }
 //                                else
 //                                {
 //                                        ptr += len;
 //                                        left -= len;
 //                                }
 //                        }

 //                        /*

}

int read_message(int port, message_t *msg) {
	uint16_t buf[1024];

	/* read the header */
	if(read(port, buf, 2) != 2) return -1;
	msg->hdr.len = *buf;
	if(read(port, buf, 2) != 2) return -1;
	msg->hdr.src = *buf;
	if(read(port, buf, 2) != 2) return -1;
	msg->hdr.typ = *buf;
	
	/* read the raw data */
	if (msg->hdr.len) {
		msg->raw = (uint8_t *) malloc(msg->hdr.len);
		read(port, msg->raw, msg->hdr.len);
	}

	/* don't print this info!
	printf("received %d bytes\n", 6+msg->hdr.len); */

	return 6 + msg->hdr.len;
}


void print_message_header(message_t *msg) {
	printf("HEADER:\n\tlen: 0x%04x (%d)\n\tsrc: 0x%04x (%d)\n\ttyp: 0x%04x (%d)\n",
		msg->hdr.len, msg->hdr.len, msg->hdr.src, msg->hdr.src, msg->hdr.typ, msg->hdr.typ);
}



void disconnect(int signum) { close(intFH); }


int read_from_raw(uint8_t **r, uint16_t *v) {
	parse_from_raw(*r, v);
	*r += sizeof(*v);
	return 0;
}

int parse_from_raw(const uint8_t *r, uint16_t *v) {
	// printf("%02x%02x %d", r[0], r[1], *((uint16_t*)r));
	//((uint16_t)r[1]) << 8 | (uint16_t) r[0];
	*v = *((uint16_t*)r); 
	return 0;
}

// int parse_string_from_raw(const uint8_t *r, char *s, int len) {


// 	int i;
// 	for(i = 0; i < len; i++) {
// 		s[i] = r[i];
// 	}
// 	s[i] = '\0';
// 	return i;
// }

int parse_stdstring_from_raw(const uint8_t *r, char **p_in) {
	uint8_t len = *r; 
	int i;
	char *s = (*p_in) = (char *) malloc(len + 1);
	for(i = 0; i < len; i++) {
		s[i] = r[i+1];
	}
	s[i++] = '\0';
	return i;
}



int parse_desc_reply(message_t *msg, uint16_t *n_named_vars,
		uint16_t *n_local_events, uint16_t *n_native_funcs)
{
	printf("Parsing description message:\n");
	/* read the name of the robot */
	char *s; 
	int bytes_read = parse_stdstring_from_raw(msg->raw, &s);
	printf("\tname: %s (%d)\n", s, bytes_read);	
	free(s);	/* cleanup */

	/* read the protocol version */
	uint16_t t;
	parse_from_raw(msg->raw + bytes_read, &t);
	printf("\tprotocol v: %u\n", t);
	bytes_read += 2;

	/* read the bytecode size */
	parse_from_raw(msg->raw + bytes_read, &t);
	printf("\tbytecode size: %u\n", t);
	bytes_read += 2;

	/* read the stack size */
	parse_from_raw(msg->raw + bytes_read, &t);
	printf("\tstack size: %u\n", t);
	bytes_read += 2;
		
	/* read the number of all variables */
	parse_from_raw(msg->raw + bytes_read, &t);
	printf("\tvariables size: %u\n", t);
	bytes_read += 2;
		
	/* read the number of all named variables */
	parse_from_raw(msg->raw + bytes_read, &t);
	printf("\tnamed variables size: %u\n", t);
	bytes_read += 2;
	*n_named_vars = t;
	/* the named variables are sent separately and need to parsed later */

	/* read the number of all local events */
	parse_from_raw(msg->raw + bytes_read, &t);
	printf("\tlocal events size: %u\n", t);
	bytes_read += 2;
	*n_local_events = t;
	/* the local events are sent separately and need to parsed later */
		
	/* read the number of all native functions (e.g. leds, sound ...) */
	parse_from_raw(msg->raw + bytes_read, &t);
	printf("\tnative functions size: %u\n", t);
	bytes_read += 2;
	*n_native_funcs = t;
	/* the native functions are sent separately and need to parsed later */

	printf("%d bytes read of %d\n", bytes_read, msg->hdr.len);

	return 0;
}


/* reading this messages and print out the variables available
   but dropping them immediatley */
int read_named_variables(int port, uint16_t cnt) {
	uint16_t n_variables; char* s = NULL;
	message_t msg = {{0,0,0}, NULL};

	while(cnt-- > 0) {
		read_message(port, &msg);

		/* read the variable name into s */
		parse_from_raw(msg.raw, &n_variables);
		parse_stdstring_from_raw(msg.raw + 2, &s);
		printf("\n\tvariable name: %s (%d)\t", s, n_variables);
		if(s) free(s);	/* cleanup */

		if(msg.raw) free(msg.raw);
	}
	return 0;
}

/* reading this messages but dropping them immediatley */
int read_local_events(int port, uint16_t cnt) {
	message_t msg = {{0,0,0}, NULL};
	while(cnt-- > 0) {
		read_message(port, &msg);
		if(msg.raw) free(msg.raw);
	}
	return 0;	
}

/* reading this messages but dropping them immediatley */
int read_native_functions(int port, uint16_t cnt) {
	message_t msg = {{0,0,0}, NULL};
	uint16_t bytes_parsed;
	char *s = NULL;
	while(cnt-- > 0) {
		read_message(port, &msg);

		/* read the function name into s */
		bytes_parsed = parse_stdstring_from_raw(msg.raw, &s);
		printf("\n\tnative function:\n\t\tname: %s\t", s);

		if(s) free(s);	/* cleanup */

		/* read the function description into s */
		bytes_parsed = parse_stdstring_from_raw(msg.raw + bytes_parsed, &s);
		// UTF8ToWString(msg.raw, msg.hdr.len, s);
		printf("\n\t\tdesc: %s (%d)\t", s, bytes_parsed);
		if(s) free(s);

		
		if(msg.raw) free(msg.raw);
	}
	return 0;	
}


int send_reboot_msg(int port) {
	message_t msg = {{0, 0, 0}, NULL};

	/* clear input buffer */

	/* build message structure */
	uint16_t data = THYMIO_ID;
	create_message(&msg, ASEBA_MESSAGE_REBOOT, &data, 2);

	/* write some bytes */
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

	// uint16_t value;
	// parse_from_raw(msg.raw, &value);
	// printf("value of #%d is: %d\n", 0, value);
	// parse_from_raw(msg.raw + 2, &value);
	// printf("value of #%d is: %d\n", 1, value);

	return 0;
}


int send_get_desc_msg(int port) {
	message_t msg = {{0, 0, 0}, NULL};
	uint16_t n_named_vars, n_local_events, n_native_funcs;

	/* build message structure */
	uint16_t data = (uint16_t)ASEBA_PROTOCOL_VERSION;
	create_message(&msg, ASEBA_MESSAGE_GET_DESCRIPTION, &data, 2);
	write_message(port, &msg);
	free(msg.raw);		/* Cleanup! */
	
	usleep(SLEEP_MS*10);
	/* read some bytes */
	read_message(port, &msg);	
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
	printf("reading native functions... ");
	read_native_functions(port, n_native_funcs);
	printf("done!\n");


	printf("Done parsing description messages!\n");
	return 0;
}

int send_get_vars_msg(int port,  uint16_t idx, uint16_t n_values, message_t *msg) {
	/* build message structure */
	uint16_t data[3];
	data[0] = (uint16_t) THYMIO_ID;
	data[1] = (uint16_t) idx;
	data[2] = (uint16_t) n_values;
	create_message(msg, ASEBA_MESSAGE_GET_VARIABLES, &data, sizeof(data));
	write_message(port, msg);
	// print_message_header(msg);
	free(msg->raw);		/* Cleanup! */
	
	usleep(SLEEP_MS*10);

	/* read some bytes */
	printf("received %d bytes\n", read_message(port, msg));
	// print_message_header(msg);
	return 0;
}

int send_set_vars_msg(int port,  uint16_t idx, uint16_t *values, uint16_t n_values) {
	message_t msg = {{0, 0, 0}, NULL};
	int i;

	/* build message structure */
	uint16_t len = sizeof(uint16_t) * (2 + n_values);
	uint16_t *data = (uint16_t*) malloc(len);
	//uint16_t data = [3]; 
	data[0] = (uint16_t) THYMIO_ID;
	data[1] = (uint16_t) idx;
	for(i = 0; i < n_values; i++)
		data[2+i] =  values[i];

	create_message(&msg, ASEBA_MESSAGE_SET_VARIABLES, data, len);
	write_message(port, &msg);
	//print_message_header(&msg);
	free(msg.raw);	/* Cleanup! */
	free(data);		/* Cleanup! */	
			
	/* the set vars message does not yield a reply */

	return 0;
}

uint16_t* swap_endian(uint16_t *buf, int n) {
	int i = 0;
	for(;i < n;i++) buf[i] = (buf[i] >> 8) | (buf[i] << 8);
	return buf;
}
