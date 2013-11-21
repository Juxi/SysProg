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

#ifndef _THYMIO_H_
#define _THYMIO_H_

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>	// for open, close

#include <unistd.h>	// for read
#include <signal.h>
#include <termios.h> // for settings
#include <string.h> // for memset

// Aseba Message struture -> little endian!!
// see https://aseba.wikidot.com/forum/t-655386/labview-interface#post-1800691
typedef struct message {
	struct {
		uint16_t len;
		uint16_t src;
		uint16_t typ;
	} hdr;

	uint8_t *raw;
} message_t;

#define ASEBA_DEST_DEBUG 0
#define	SLEEP_MS 10000 		/* 10 ms */
#define THYMIO_ID 1
#define ASEBA_MAX_EVENT_ARG_SIZE (2*258)

enum {
	/* from IDE to all nodes */
	ASEBA_MESSAGE_GET_DESCRIPTION = 0xA000,
	
	/* from IDE to a specific node */
	ASEBA_MESSAGE_SET_BYTECODE,
	ASEBA_MESSAGE_RESET,
	ASEBA_MESSAGE_RUN,
	ASEBA_MESSAGE_PAUSE,
	ASEBA_MESSAGE_STEP,
	ASEBA_MESSAGE_STOP,
	ASEBA_MESSAGE_GET_EXECUTION_STATE,
	ASEBA_MESSAGE_BREAKPOINT_SET,
	ASEBA_MESSAGE_BREAKPOINT_CLEAR,
	ASEBA_MESSAGE_BREAKPOINT_CLEAR_ALL,
	ASEBA_MESSAGE_GET_VARIABLES,
	ASEBA_MESSAGE_SET_VARIABLES,
	ASEBA_MESSAGE_WRITE_BYTECODE,
	ASEBA_MESSAGE_REBOOT,
	ASEBA_MESSAGE_SUSPEND_TO_RAM,
};

/* Robot Communication Functions*/
int connect(const char *port_name);
int configure(int port);
int read_message(int port, message_t *msg);
int write_message(int port, message_t *msg);


/* Global variable  */
int intFH; /* keep a copy of the port handle for the interrupt signal */

/* CTRL-C signal handler and disconnecter */
void disconnect(int signum);

/*	swap each element's bytes
	this function is actually not needed here */
uint16_t* swap_endian(uint16_t *buf, int n);

#endif