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



/* warning this destroys the message! */
int write_message(int port, message_t *msg) {
	// TODO:
	if (msg->hdr.len > ASEBA_MAX_EVENT_ARG_SIZE) {	// 258 * 2 ?! from consts.h
		fprintf(stderr, "fatal error: message size exceed maximum packet size.\n");
		fprintf(stderr, "message payload size: %d\n", msg->hdr.len);
		fprintf(stderr, "maximum packet payload size: %d\n", ASEBA_MAX_EVENT_ARG_SIZE);
		return -1;
	}
	int written_bytes = 0, h;

	//printf("HEADER: %04x %04x %04x\n", msg->hdr.len, msg->hdr.src, msg->hdr.typ);
	
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
	long n, dotcount = 0, sum = 0;

	/* read the header */
	if(read(port, buf, 2) != 2) return -1;
	msg->hdr.len = *buf;
	if(read(port, buf, 2) != 2) return -1;
	msg->hdr.src = *buf;
	if(read(port, buf, 2) != 2) return -1;
	msg->hdr.typ = *buf;
	
	if (msg->hdr.len) {
		msg->raw = (uint8_t *) malloc(msg->hdr.len);
		read(port, msg->raw, msg->hdr.len);
	}

	printf("received %d bytes\n", 6+msg->hdr.len);

	/*  maybe try this here?
if (size == 0)
                                return;
                        
                        char *ptr = (char *)data;
                        size_t left = size;
                        
                        while (left)
                        {
                                ssize_t len = ::read(fd, ptr, left);
                                
                                if (len < 0)
                                {
                                        fail(DashelException::IOError, errno, "File read I/O error.");
                                }
                                else if (len == 0)
                                {
                                        fail(DashelException::ConnectionLost, 0, "Reached end of file.");
                                }
                                else
                                {
                                        ptr += len;
                                        left -= len;
                                }
                        }
	/**/
	return 6 + msg->hdr.len;
}


uint16_t* swap_endian(uint16_t *buf, int n) {
	int i = 0;
	for(;i < n;i++) buf[i] = (buf[i] >> 8) | (buf[i] << 8);
	return buf;
}

void disconnect(int signum) { close(intFH); }

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


// from dashel/aseba studio
// void connect() {
// //				std::string proto, params;
//                 char *proto = "ser";
//                 char *params = "name=Thymio-II";
                
//                 SelectableStream *s(dynamic_cast<SelectableStream*>(
//                 	streamTypeRegistry.create(proto, target, *this)));
//                 // calls creatorFunc --> new SerialStream()
//                 	// this creates
//                 if(!s)
//                 {
//                         std::string r = "Invalid protocol in target: ";
//                         r += proto;
//                         r += ", known protocol are: ";
//                         r += streamTypeRegistry.list();
//                 }
                
//                 /* The caller must have the stream lock held */
                
//                 streams.insert(s);
//                 dataStreams.insert(s);
//                 connectionCreated(s);
//                 //return s;
// }