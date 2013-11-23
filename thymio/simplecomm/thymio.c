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

#include <ctype.h>
#include "thymio.h"

int connect(const char *port_name) {
	int port;

	/* enable CTRL-C callback which should close the file */
	signal(SIGINT,disconnect);	

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


uint16_t* swap_endian(uint16_t *buf, int n) {
	int i = 0;
	for(;i < n;i++) buf[i] = (buf[i] >> 8) | (buf[i] << 8);
	return buf;
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

int parse_string_from_raw(const uint8_t *r, char *s, int len) {
	int i;
	for(i = 0; i < len; i++) {
		s[i] = r[i];
	}
	s[i] = '\0';
	return i;
}



int parse_desc_reply(message_t *msg, uint16_t *n_named_vars,
		uint16_t *n_local_events, uint16_t *n_native_funcs)
{
	printf("Parsing description message:\n");
		// from the other side:
		// add(WStringToUTF8(name));
	char *s = (char*) malloc(msg->hdr.len);
	// uint8_t *h = msg->raw;	// for backup
	int bytes_read = parse_string_from_raw(msg->raw, s, msg->hdr.len - 14);
	printf("\tname: %s (%d)\n", s, bytes_read);
	free(s);

		// add(static_cast<uint16>(protocolVersion));
	uint16_t t;
	parse_from_raw(msg->raw + bytes_read, &t);
	printf("\tprotocol v: %u\n", t);
	bytes_read += 2;

		// add(static_cast<uint16>(bytecodeSize));
	parse_from_raw(msg->raw + bytes_read, &t);
	printf("\tbytecode size: %u\n", t);
	bytes_read += 2;

		// add(static_cast<uint16>(stackSize));
	parse_from_raw(msg->raw + bytes_read, &t);
	printf("\tstack size: %u\n", t);
	bytes_read += 2;
		// add(static_cast<uint16>(variablesSize));
	parse_from_raw(msg->raw + bytes_read, &t);
	printf("\tvariables size: %u\n", t);
	bytes_read += 2;
		
		// add(static_cast<uint16>(namedVariables.size()));
	parse_from_raw(msg->raw + bytes_read, &t);
	printf("\tnamed variables size: %u\n", t);
	bytes_read += 2;
	*n_named_vars = t;
		// // named variables are sent separately

		// add(static_cast<uint16>(localEvents.size()));
	parse_from_raw(msg->raw + bytes_read, &t);
	printf("\tlocal events size: %u\n", t);
	bytes_read += 2;
	*n_local_events = t;
		// // local events are sent separately
		
		// add(static_cast<uint16>(nativeFunctions.size()));
	parse_from_raw(msg->raw + bytes_read, &t);
	printf("\tnative functions size: %u\n", t);
	bytes_read += 2;
	*n_native_funcs = t;
		// native functions are sent separately

	printf("%d bytes read of %d\n", bytes_read, msg->hdr.len);

	return 0;
}


/* reading this messages and print out the variables available
   but dropping them immediatley */
int read_named_variables(int port, uint16_t cnt) {
	uint16_t h; char* s = NULL;
	message_t msg = {{0,0,0}, NULL};

	while(cnt-- > 0) {
		read_message(port, &msg);
		parse_from_raw(msg.raw, &h);
		s = (char*) malloc(msg.hdr.len - 2);
		UTF8ToWString(msg.raw + 2, msg.hdr.len - 2, s);
		printf("\n\tvariable name: %s (%d)\t", s, h);

		// printf("descr: (%d) ", msg.hdr.len);
		// for(i = 0; i < strlen(s); i++) putchar(s[i]);

		if(s) free(s);
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
	while(cnt-- > 0) {
		read_message(port, &msg);
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
	printf("reading native functions...");
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



/* taken from Aseba source code */
void UTF8ToWString(const uint8_t *s, int len, char *out) {
	int j = 0;
	for (int i = 0; i < len; ++i)	{
		const uint8_t *a = &s[i];
		if (*a == '\n' || *a == '\t' || *a == '\r') continue;
		else if (!(*a&128)) {
			//Byte represents an ASCII character. Direct copy will do.
			if(isprint(*a)) out[j++] = *a;
		}else if ((*a&192)==128)
			//Byte is the middle of an encoded character. Ignore.
			continue;
		else if ((*a&224)==192)
			//Byte represents the start of an encoded character in the range
			//U+0080 to U+07FF
			out[j++] = ((*a&31)<<6)|(a[1]&63);
		else if ((*a&240)==224)
			//Byte represents the start of an encoded character in the range
			//U+07FF to U+FFFF
			out[j++] = ((*a&15)<<12)|((a[1]&63)<<6)|(a[2]&63);
		else if ((*a&248)==240)
			//Byte represents the start of an encoded character beyond the
			//U+FFFF limit of 16-bit integers
			out[j++] = '?';
	}
	// TODO: add >UTF16 support
	out[j++] = L'\0';
}

// int test_write_message(int port, message_t *msg) {
// this works!:)
// //	uint16_t arr[] = {0x00, 0x02, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x01};
// 	//uint16_t arr[] = {0x02, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x01, 0x00};
// //	printf("%d written\n", write(port, arr, 8));

// 	uint16_t type = ASEBA_MESSAGE_REBOOT;

// 	uint16_t len = 2;
// 	uint16_t t;
// 	write(port, &len, 2);
// 	t = 0x00;
// 	write(port, &t, 2);
// 	t = type; // msg.type;
// 	write(port, &t, 2);

// 	len = 1;
// 	uint8_t rawData[2] = { 0 };
// 	const uint8_t *ptr = (const uint8_t *)&len; //reinterpret_cast<const uint8 *>(&dest);
// 	rawData[0] = ptr[0];
// 	rawData[1] = ptr[1];

// 	write(port, &rawData[0], 2);
// }