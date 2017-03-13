/* 

Real-Time Operating Systems
Project 2b
Chaithanya Gadiyam & Michael Moffitt
*/

#ifndef PWM_H
#define PWM_H

#include "types.h"
#include <pthread.h>
#include "servo.h"
#include <stdint.h>       /* for uintptr_t */
#include <hw/inout.h>     /* for in*() and out*() functions */
#include <sys/mman.h>     /* for mmap_device_io() */
#include <sys/neutrino.h> /* for ThreadCtl() */

void pwm_init(u8 channel);
void pwm_set(u8 port_no, u8 pos);

// Period in microseconds
#define PERIOD 20000000
#define PORTA_ADDRESS 0x288
#define PORTB_ADDRESS 0x289
#define CTRL_ADDRESS 0x28B
#define PORT_LENGTH 1
#define PW_POS0 300
#define PW_POS1 600
#define PW_POS2 900
#define PW_POS3 1200
#define PW_POS4 1600
#define PW_POS5 2000
#define PWM_ADJ 50000




/*typedef struct pwm_args pwm_args;
struct pwm_args
{
	u32 pulse_width;
};
pwm_args *global_pwm_args1;
pwm_args *global_pwm_args2;*/

unsigned int Pulse_Width1;
unsigned int Pulse_Width2;


void *pwm_routine1(void *args);
void *pwm_routine2(void *args);

#endif
