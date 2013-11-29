/**
 * abd.c Aseba Bytecode Disassembler
 * TODO: 
 * - Check which version / robot /firmware is the correct one. 
 * - Possible to upload/ download from/to robot directly?
 * - address lables
 * - variable names 
 */

/**

 The source file of aseba 'aseba/clients/studio/MainWindow.cpp' is writiong the file whcih we want to read.

 Got by 'git clone git://github.com/aseba-community/aseba.git'

 ---8<---------------

	// See AS001 at https://aseba.wikidot.com/asebaspecifications

	// header
	const char* magic = "ABO";
	file.write(magic, 4);
	write16(file, 0); // binary format version
	write16(file, ASEBA_PROTOCOL_VERSION);
	write16(file, vmMemoryModel->getVariableValue("_productId"), "product identifier (_productId)");
	write16(file, vmMemoryModel->getVariableValue("_fwversion"), "firmware version (_fwversion)");
	write16(file, id);
	write16(file, crcXModem(0, nodeName));
	write16(file, target->getDescription(id)->crc());

	// bytecode
	write16(file, bytecode.size());
	uint16 crc(0);
	for (size_t i = 0; i < bytecode.size(); ++i)
	{
	        const uint16 bc(bytecode[i]);
	        write16(file, bc);
	        crc = crcXModem(crc, bc);
	}
	write16(file, crc);

	--->8---------------
 */ 

#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>

#include "consts.h"
//#include <aseba/common/consts.h>

typedef struct {uint16_t size; const char* name; } variable_t;

variable_t variables[] = {
		{1, "_id"},
		{1, "event.source"}, 
		{32, "event.args"},
		{2, "_fwversion"}, 
		{1, "_productId"}, 
		
		{5, "buttons._raw"},
		{1, "button.backward"},
		{1, "button.left"},
		{1, "button.center"},
		{1, "button.forward"},
		{1, "button.right"},
			
		{5, "buttons._mean"},
		{5, "buttons._noise"},
		
		{7, "prox.horizontal"},
		{2, "prox.ground.ambiant"},
		{2, "prox.ground.reflected"},
		{2, "prox.ground.delta"},
		
		{1, "motor.left.target"},
		{1, "motor.right.target"},
		{2, "_vbat"},
		{2, "_imot"},
		{1, "motor.left.speed"},
		{1, "motor.right.speed"},
		{1, "motor.left.pwm"},
		{1, "motor.right.pwm"},
		
		{3, "acc"},
		
		{1, "temperature"},
		
		{1, "rc5.address"},
		{1, "rc5.command"},
		
		{1, "mic.intensity"},
		{1, "mic.threshold"},
		{1, "mic._mean"},
		
		{2, "timer.period"},
		
		{1, "acc._tap"}
};

const char* variable_name_of_index(uint16_t idx)
{
	static char ret[256];
	size_t num = sizeof(variables)/sizeof(variable_t);
	size_t i;
	uint16_t sum = 0;
	for (i=0;i<num;i++)
	{
		if (idx<=sum)
		{
			if (variables[i].size>1)
			{
				sprintf(ret,"%s[%d]",variables[i].name,sum-idx);
			}
			else
			{
				sprintf(ret,"%s",variables[i].name);
			}	
			return ret;
		}
      	sum += variables[i].size;
	}
	return "unknown";
}

const char *native[] = {
        "AsebaNative_system_reboot",
        "AsebaNative_system_settings_read",
        "AsebaNative_system_settings_write",
        "AsebaNative_system_settings_flash",

        "AsebaNative_math_veccopy",
        "AsebaNative_math_vecfill",
        "AsebaNative_math_vecaddscalar",
        "AsebaNative_math_vecadd",
        "AsebaNative_math_vecsub",
        "AsebaNative_math_vecmul",
        "AsebaNative_math_vecdiv",
        "AsebaNative_math_vecmin",
        "AsebaNative_math_vecmax",
        "AsebaNative_math_vecclamp",
        "AsebaNative_math_vecdot",
        "AsebaNative_math_vecstat",
        "AsebaNative_math_vecargbounds",
        "AsebaNative_math_vecsort",
        "AsebaNative_math_mathmuldiv",
        "AsebaNative_math_mathatan2",
        "AsebaNative_math_mathsin",
        "AsebaNative_math_mathcos",
        "AsebaNative_math_mathrot2",
        "AsebaNative_math_mathsqrt",
        "AsebaNative_math_rand",
        
        "ThymioNative_set_led",
        "ThymioNative_sound_record",
        "ThymioNative_sound_playback",
        "ThymioNative_sound_replay",
        "ThymioNative_sound_system",
        "ThymioNative_set_led_circle",
        "ThymioNative_set_rgb_top",
        "ThymioNative_set_rgb_bl",
        "ThymioNative_set_rgb_br",
        "ThymioNative_play_freq",
        "ThymioNative_set_buttons_leds",
        "ThymioNative_set_hprox_leds",
        "ThymioNative_set_vprox_leds",
        "ThymioNative_set_rc_leds",
        "ThymioNative_set_sound_leds",
        "ThymioNative_set_ntc_leds",
        "ThymioNative_set_wave",

        "AsebaNative_power_off"
};

const char* event_names[] = {
	"(start)",          // -1 -~-> 0
    "button.backward",  // -2 -~-> 1
    "button.left",      
    "button.center",    // -4
    "button.forward",   // -5
    "button.right",     
    "buttons",           
    "prox",
    "tap",
    "acc",
    "mic",
    "sound.finished",
    "temperature",
    "rc5",
    "motor",
    "timer0",
    "timer1"
};

typedef struct {const char* name; const char* desc; AsebaBytecodeId id; } bytecode_name_t;

bytecode_name_t bytecode_name[] = {
	"stop", "halts the program or 'return' from event handling", ASEBA_BYTECODE_STOP,
    "small immediate", "push on stack", ASEBA_BYTECODE_SMALL_IMMEDIATE,
    "large immediate", "push next on stack", ASEBA_BYTECODE_LARGE_IMMEDIATE,
    "load", "push var value on stack", ASEBA_BYTECODE_LOAD,
    "store", "pop var value from stack", ASEBA_BYTECODE_STORE,
    "load indirect", "replace top stack index with value next has array size", ASEBA_BYTECODE_LOAD_INDIRECT,
    "store indirect", "pop value index from stack next has array size", ASEBA_BYTECODE_STORE_INDIRECT,
    "unary arithmetic", "rw on stack element", ASEBA_BYTECODE_UNARY_ARITHMETIC,
    "binary arithmetic", "two pop one push", ASEBA_BYTECODE_BINARY_ARITHMETIC,
    "jump", "relative", ASEBA_BYTECODE_JUMP,
    "cond branch", "relative by next code if binary cond on stack is false", ASEBA_BYTECODE_CONDITIONAL_BRANCH,
    "emit", "next two are var and length, arg is eventid", ASEBA_BYTECODE_EMIT,
    "native call", "arguments indirect on stack", ASEBA_BYTECODE_NATIVE_CALL,
    "sub call", "push next on stack jump abs", ASEBA_BYTECODE_SUB_CALL,
    "sub ret", "pop from stack abs jump", ASEBA_BYTECODE_SUB_RET,
    "unknown", "unknown", 0xF
};

const char *bin_op_name[] = {
	"SHIFT_LEFT",
	"SHIFT_RIGHT",
	"ADD",
	"SUB",
	"MULT",
	"DIV",
	"MOD",
	// binary arithmetic
	"BIT_OR",
	"BIT_XOR",
	"BIT_AND",
	// comparison
	"EQUAL",
	"NOT_EQUAL",
	"BIGGER_THAN",
	"BIGGER_EQUAL_THAN",
	"SMALLER_THAN",
	"SMALLER_EQUAL_THAN",
	// logic
	"OR",
	"AND"
};

const char *unary_op_name[] = {
	"SUB",
	"ABS",
	"BIT_NOT",
	"NOT (unused)"	// < not used for the VM, only for parsing, reduced by compiler using de Morgan and comparison inversion};
};

int main(int argc, char *argv[])
{
	uint16_t code;
	uint16_t nextcode;
	ssize_t next;
	ssize_t counter=0;
	ssize_t numnative = sizeof(native) / sizeof(const char*);
	uint16_t events = 0;
	uint16_t succeeding = 0;
	next=read(fileno(stdin),&nextcode,2);
	while(2==next)
	{
		code = nextcode;
		next = read(fileno(stdin),&nextcode,2);
		if (next==0) // EOF
		{
			printf("                 %17s\n","---Bytecode Check---");
			printf("%04zx %02x %02x: %17s 0x%04x (%d) %s\n",counter-0x00a,(code>>8)&0xff,code&0xff,"last word",code,code,"(crc-16 check sum only of bytecode)");
			break;
		}
		uint16_t opcode = (code>>12);
		uint16_t argument = code & 0x0fff;
		int16_t sargument = ((int16_t)(code << 4)) >> 4;
		if (counter<0x00a) // Header whaever ...
		{
			switch(counter)
			{
				case 0x00:
					printf("                 %17s\n","---Header-----------");
					printf("%04zx %02x %02x: %17s 0x%04x '%c''%c'\n",counter,(code>>8)&0xff,code&0xff,"magic 1",code,isprint((code>>8)&0xff)?(code>>8)&0xff:' ',isprint(code&0xff)?code&0xff:' ');			
					break;
				case 0x01:
					printf("%04zx %02x %02x: %17s 0x%04x '%c''%c'\n",counter,(code>>8)&0xff,code&0xff,"magic 2",code,isprint((code>>8)&0xff)?(code>>8)&0xff:' ',isprint(code&0xff)?code&0xff:' ');			
					break;
				case 0x02:
					printf("%04zx %02x %02x: %17s 0x%04x (%d)\n",counter,(code>>8)&0xff,code&0xff,"binary format ver",code,code);			
					break;
				case 0x03:
					printf("%04zx %02x %02x: %17s 0x%04x (%d)\n",counter,(code>>8)&0xff,code&0xff,"protocol ver",code,code);			
					break;
				case 0x04:
					printf("%04zx %02x %02x: %17s 0x%04x (%d)\n",counter,(code>>8)&0xff,code&0xff,"prod identifier",code,code);			
					break;
				case 0x05:
					printf("%04zx %02x %02x: %17s 0x%04x (%d)\n",counter,(code>>8)&0xff,code&0xff,"firmware version",code,code);			
					break;
				case 0x06:
					printf("%04zx %02x %02x: %17s 0x%04x (%d)\n",counter,(code>>8)&0xff,code&0xff,"id",code,code);			
					break;
				case 0x07:
					printf("%04zx %02x %02x: %17s 0x%04x (%d)\n",counter,(code>>8)&0xff,code&0xff,"crc of nodeName",code,code);			
					break;
				case 0x08:
					printf("%04zx %02x %02x: %17s 0x%04x (%d)\n",counter,(code>>8)&0xff,code&0xff,"crc of descr",code,code);			
					break;
				case 0x09:
					printf("                 %17s\n","---Bytecode Size----");
					printf("%04zx %02x %02x: %17s 0x%04x (%d) ; in words\n",counter,(code>>8)&0xff,code&0xff,"bytcode size",code,code);			
					break;
			}
		}
		else if (counter==0x00a)
		{
			printf("                 %17s\n","---Event Table------");
			printf("%04zx %02x %02x: %17s 0x%04x (%d)\n",counter-0x00a,(code>>8)&0xff,code&0xff,"event block size",code,code);
			events = code;
		}
		else if (counter<0x00a+events)
		{
			if ((counter-0x00a-events)%2==0)
			{
				if ((sizeof(event_names)/sizeof(const char*))>((uint16_t)(~code)))
				{
					printf("%04zx %02x %02x: %17s 0x%04x (%d/%d/~%d) %s\n",counter-0x00a,(code>>8)&0xff,code&0xff,"event type",code,code,(int16_t)code,(uint16_t)(~code),event_names[(uint16_t)(~code)]);
				}
				else
				{
					printf("%04zx %02x %02x: %17s 0x%04x (%d/%d/~%d) %s\n",counter-0x00a,(code>>8)&0xff,code&0xff,"event type",code,code,(int16_t)code,(uint16_t)(~code),"unknown");					
				}
			}
			else
			{
				printf("%04zx %02x %02x: %17s 0x%04x (%d)\n",counter-0x00a,(code>>8)&0xff,code&0xff,"event address",code,code);
			}
		}
		else 
		{ 
			if (counter==0x00a+events)
			{
				printf("                 %17s\n","---Bytecode Data----");
			}	
			if (succeeding)
			{
				printf("%04zx %02x %02x: %17s 0x%04x (%d/%d)\n",counter-0x00a,(code>>8)&0xff,code&0xff,"argument",code,code,(int16_t)code);
				succeeding--;	
			}
			else if (opcode==ASEBA_BYTECODE_NATIVE_CALL)
			{
				if (numnative>argument)
				{
     			   	// Why is the argument-1????
					printf("%04zx %01x %03x: %17s %s (%d)\n",counter-0x00a,opcode,argument,bytecode_name[opcode].name,native[argument],argument);
				}
				else
				{
					printf("%04zx %01x %03x: %17s %s (%d)\n",counter-0x00a,opcode,argument,bytecode_name[opcode].name,"(undefined)",argument);
				}
			}
			else if (opcode== ASEBA_BYTECODE_BINARY_ARITHMETIC || opcode== ASEBA_BYTECODE_CONDITIONAL_BRANCH )
			{
				if ((sizeof(bin_op_name)/sizeof(const char*))>(argument&ASEBA_BINARY_OPERATOR_MASK))
				{
					printf("%04zx %01x %03x: %17s %s (%d)\n",counter-0x00a,opcode,argument,bytecode_name[opcode].name,bin_op_name[argument&ASEBA_BINARY_OPERATOR_MASK],argument&ASEBA_BINARY_OPERATOR_MASK);
				}
				else
				{
					printf("%04zx %01x %03x: %17s %s (%d)\n",counter-0x00a,opcode,argument,bytecode_name[opcode].name,"unknown",argument&ASEBA_BINARY_OPERATOR_MASK);
				}
			}
			else if (opcode== ASEBA_BYTECODE_UNARY_ARITHMETIC)
			{
				if ((sizeof(unary_op_name)/sizeof(const char*))>(argument&ASEBA_UNARY_OPERATOR_MASK))
				{
					printf("%04zx %01x %03x: %17s %s (%d)\n",counter-0x00a,opcode,argument,bytecode_name[opcode].name,unary_op_name[argument&ASEBA_UNARY_OPERATOR_MASK],argument&ASEBA_UNARY_OPERATOR_MASK);
				}
				else
				{
					printf("%04zx %01x %03x: %17s %s (%d)\n",counter-0x00a,opcode,argument,bytecode_name[opcode].name,"unknown",argument&ASEBA_UNARY_OPERATOR_MASK);
				}
			}
			else if (opcode==ASEBA_BYTECODE_STORE || opcode==ASEBA_BYTECODE_LOAD)
			{
				const char* name = variable_name_of_index(argument);
				printf("%04zx %01x %03x: %17s %s (%d)\n",counter-0x00a,opcode,argument,bytecode_name[opcode].name,name,argument);
			}
			else
			{
				printf("%04zx %01x %03x: %17s %d (%d)\n",counter-0x00a,opcode,argument,bytecode_name[opcode].name,argument,sargument);
			}
			if (!succeeding)
			{
				switch (opcode)
				{
					case ASEBA_BYTECODE_LARGE_IMMEDIATE:
					case ASEBA_BYTECODE_LOAD_INDIRECT:
					case ASEBA_BYTECODE_STORE_INDIRECT:
					case ASEBA_BYTECODE_CONDITIONAL_BRANCH:
						succeeding = 1;
					break;
					case ASEBA_BYTECODE_EMIT:
						succeeding = 2;
					break;
					// default: // We stay at 0 with succeeding					
				}
			}
		};
		counter++;
	};
	if (next==-1)
	{
		perror("reading in bytecode");
		return errno;
	}
	return EXIT_SUCCESS;
}
