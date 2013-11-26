/*************************************************************
 * Simple Thymio USB Communication Library
 * thymio.h
 *
 * Systems Programming BSc course    (2013)
 * Universita della Svizzera Italiana (USI)
 * 
 * author: Juxi Leitner <juxi@idsia.ch>, http://Juxi.net/
 *         Alexander Förster <alexander@idsia.ch>
 ************************************************************/

#ifndef _THYMIO_H_
#define _THYMIO_H_

#include <stdint.h>
/* Aseba Message struture -> little endian!!
 * see https://aseba.wikidot.com/forum/t-655386/labview-interface#post-1800691 */
typedef struct message {
	struct {
		uint16_t len;
		uint16_t src;
		uint16_t typ;
	} hdr;

	uint8_t *raw;
} message_t;

#define THYMIO_LIBRARY_VERSION 4	/* 17:00 @ Nov 26, 2013 */
#define ASEBA_DEST_DEBUG 0
#define ASEBA_PROTOCOL_VERSION 4
#define	SLEEP_MS 10000          /* 10 ms */
#define THYMIO_ID 1
#define ASEBA_MAX_EVENT_ARG_SIZE (2*258)

/* constants taken from Aseba */
enum {
	/* from a specific node */
	ASEBA_MESSAGE_DESCRIPTION = 0x9000,
	ASEBA_MESSAGE_NAMED_VARIABLE_DESCRIPTION,
	ASEBA_MESSAGE_LOCAL_EVENT_DESCRIPTION,
	ASEBA_MESSAGE_NATIVE_FUNCTION_DESCRIPTION,
	ASEBA_MESSAGE_DISCONNECTED,
	ASEBA_MESSAGE_VARIABLES,
	ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS,
	ASEBA_MESSAGE_DIVISION_BY_ZERO,
	ASEBA_MESSAGE_EVENT_EXECUTION_KILLED,
	ASEBA_MESSAGE_NODE_SPECIFIC_ERROR,
	ASEBA_MESSAGE_EXECUTION_STATE_CHANGED,
	ASEBA_MESSAGE_BREAKPOINT_SET_RESULT,
		
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


/* Low Level Robot Communication Functions */
int connect(const char *port_name);
int configure(int port);

/* message functions */
void create_message(message_t *msg, uint16_t type, void *data, uint16_t len);
int read_message(int port, message_t *msg);
int write_message(int port, message_t *msg);
void print_message_header(message_t *msg);

/* parse content from the rawdata in the message */
int parse_from_raw(const uint8_t *r, uint16_t *v);
int parse_string_from_raw(const uint8_t *r, char **s);
/* equal to parse from raw but also moves the pointer! */
int read_from_raw(uint8_t **r, uint16_t *v);

/* mid level communication functions */
int parse_desc_reply(message_t *msg, uint16_t *n_named_vars,
		uint16_t *n_local_events, uint16_t *n_native_funcs);


/* higher level communication functions */
int send_reboot_msg(int port);
int send_get_desc_msg(int port);
	int read_named_variables(int port, uint16_t cnt);
	int read_local_events(int port, uint16_t cnt);
	int read_native_functions(int port, uint16_t cnt);
		int read_function_parameters(uint8_t *raw, uint16_t cnt);

int send_get_vars_msg(int port, uint16_t idx, uint16_t n_values, message_t *msg);
int send_set_vars_msg(int port, uint16_t idx, uint16_t *values, uint16_t n_values);

/* Aseba program execution control */
int send_run_msg(int port);
int send_stop_msg(int port);


/* CTRL-C signal handler and disconnecter */
void disconnect(int signum);

/*	swap each element's bytes
	this function is actually not needed here */
uint16_t* swap_endian(uint16_t *buf, int n);

#endif