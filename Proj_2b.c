
/*
Real-Time Operating Systems
Project 2b
Chaithanya Gadiyam & Michael Moffitt
*/

// system includes
#include <stdio.h>
#include <stdlib.h>

// project includes
#include "types.h"
#include "servo.h"
#include "state.h"
#include "opcode.h"
#include "pwm.h"

extern pthread_t run_thread;

// Servo thread routine 
void *run_routine(void *args);

#define INIT_CHANNEL 2

#define OP_RECIPE_END 0x00
#define OP_MOV 0x01
#define OP_WAIT 0x02
#define OP_ERR1 0x03
#define OP_LOOP_START 0x04
#define OP_END_LOOP 0x05
#define OP_ERR2 0x07
#define OP_ERR3 0x06


pthread_t run_thread;

// Servo Thread routine
void *run_routine(void *args)
{
	printf("servos thread \n");
	u8 i;
	for(;;)
	{
		usleep(10000);
		// Run the kernel on each servo
		for (i = 0; i < NUM_SERVOS; i++)
		{
			servo_kernel((servo *)(global_servos + i));
		}
	}
}

void print_recipe_line(u8 data)
{
	switch (get_opcode(data))
	{
		case OP_RECIPE_END:
			printf("RECIPE_END ");
			break;
		case OP_MOV:
			printf("MOV        ");
			break;
		case OP_WAIT:
			printf("WAIT       ");
			break;
		case OP_ERR1:
			printf("MOVC       ");
			break;
		case OP_LOOP_START:
			printf("LOOP_START ");
			break;
		case OP_END_LOOP:
			printf("END_LOOP   ");
			break;
		case OP_ERR2:
			printf("INVALID 06 ");
			break;
		case OP_ERR3:
			printf("INVALID 07 ");
			break;
	}
	printf("%X\n",get_param(data));
}

void print_recipe(u8 *recipe)
{
	int i;
	for(i = 0; i < 255; i++)
	{
		print_recipe_line(recipe[i]);
		if (get_opcode(recipe[i]) == OP_RECIPE_END)
		{
		 	return;
		}
	}
}

// All programs are null terminated (same as OP_RECIPE_END)
//example recipes
const char prog0[] = { 0x21, 0x42, 0x22, 0x82, 0x21, 0x82, 0x23, 0x25, 0x50, 0x00 };
const char prog1[] = { 0x21, 0x23, 0x40, 0x22, 0x82, 0x21, 0x21, 0x24, 0x50, 0x5F, 0x00 };

// examples recipes
const char example0[] = {
	(OP_MOV << 5) | 0x00,
	(OP_MOV << 5) | 0x05,
	(OP_MOV << 5) | 0x00,
	(OP_RECIPE_END)
};

const char example1[] = {
	(OP_MOV << 5) | 0x03,
	(OP_LOOP_START << 5) | 0x08,
	(OP_MOV << 5) | 0x01,
	(OP_MOV << 5) | 0x04,
	(OP_END_LOOP << 5),
	(OP_MOV  << 5) | 0x00,
	(OP_RECIPE_END)
};

const char example2[] = {
	(OP_MOV << 5) | 0x02,
	(OP_MOVC << 5) | 0x05,
	(OP_WAIT << 5) | 0x00,
	(OP_MOV << 5) | 0x01,
	(OP_MOVC << 5) | 0x03,
	(OP_RECIPE_END)
};

const char example3[] = {
	(OP_MOV << 5) | 0x02,
	(OP_MOV << 5) | 0x03,
	(OP_LOOP_START << 5) | 0x08,
	(OP_LOOP_START << 5) | 0x03,
	(OP_MOV << 5) | 0x04,
	(OP_MOV << 5) | 0x02,
	(OP_END_LOOP << 5),
	(OP_END_LOOP << 5),
	(OP_RECIPE_END)
};

int main(int argc, char **argv)
{
	pthread_t pwm_thread1;
	pthread_t pwm_thread2;
	servo servos[NUM_SERVOS];

	u8 i;
	u8 input_num;
	char user_input;
	char *usr = ( char *)malloc(sizeof(char) * (NUM_SERVOS));

	global_servos = (servo *)malloc(sizeof(servo) * NUM_SERVOS);
	input_num = 0;
	Pulse_Width1 = 0;
	Pulse_Width2 = 0;
	global_servos = servos;

	for (i = 0; i < NUM_SERVOS; i++)
	{
		// Start assigning channels
		servo_init(&(servos[i]),i);
		pwm_init(servos[i].channel_num);
	}

	// Set up servos with the program and print out status
	servo_set_recipe(&(servos[0]), &example3[0]);
	servo_print_state(&(servos[0]));
	print_recipe(servos[0].recipe);

	servo_set_recipe(&(servos[1]), &example2[0]);
	servo_print_state(&(servos[1]));
	print_recipe(servos[1].recipe);

    struct sched_param params;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	//set highest priority to both pwm threads
	int max_priority = sched_get_priority_max(SCHED_RR);
	params.sched_priority = max_priority;
	pthread_attr_setschedparam(&attr, &params);

	//Start PWM threads
	pthread_create(&pwm_thread1, &attr, &pwm_routine1, &Pulse_Width1);
	pthread_create(&pwm_thread2, &attr, &pwm_routine2, &Pulse_Width2);


	params.sched_priority = max_priority-10;
	pthread_attr_setschedparam(&attr, &params);
	
	// Start up our run thread for the servos
	pthread_create(&run_thread, &attr, &run_routine, NULL);


	for (;;)
	{
		// Take user input. Record only [n] servos' worth of chars.
		// Once we have enough chars and the user hits enter, reset the
		// input position index and run the servo state modifier block.
		user_input = fgetc(stdin);
		// Echo input
		printf("%c", user_input);
		if (user_input == '\n' || user_input == '\r')
		{
			input_num = 0;
		}
		else if (input_num < NUM_SERVOS)
		{
			usr[input_num] = user_input;
			input_num++;
		}

		// Modify the servo state based on user input when user hits enter
		if (input_num == 0)
		{
			for (i = 0; i < NUM_SERVOS; i++)
			{
				usr[i] = usr[i] | 0x20;
				if (usr[i] == USER_RESTART)
				{
					servo_init((servo *)(servos + i),i);
					servos[i].state = STATE_RUN;
					printf("\nServo #%d -> STATE_RUN\n",servos[i].channel_num);
				}
				// State-specific commands
				switch(servos[i].state)
				{
					case STATE_BEGIN:
						if (usr[i] == USER_CONTINUE)
						{
							for (i = 0; i < NUM_SERVOS; i++)
							{
								servo_init((servo *)(servos + i),i);
							}
							servos[i].state = STATE_RUN;
							printf("\nServo #%d -> STATE_RUN\n",servos[i].channel_num);
						}
						break;
					case STATE_RUN:
						if (usr[i] == USER_PAUSE)
						{
							servos[i].state = STATE_PAUSE;
							printf("\nServo #%d -> STATE_PAUSE\n",servos[i].channel_num);
						}
						else if(usr[i] == USER_CONTINUE)
						{
							servos[i].state = STATE_ERROR;
							printf("\nServo #%d -> STATE_ERROR\n",servos[i].channel_num);
						}
						else
						{

						}
						break;
					case STATE_PAUSE:
						if (usr[i] == USER_LEFT &&
							servos[i].pos != SERVO_MIN_POS)
						{
							servos[i].pos -= 1;
						}
						else if (usr[i] == USER_RIGHT &&
							servos[i].pos != SERVO_MAX_POS)
						{
							servos[i].pos += 1;
						}
						else if (usr[i] == USER_CONTINUE)
						{
							servos[i].state = STATE_RUN;
							printf("\nServo #%d -> STATE_RUN\n",servos[i].channel_num);
							break;
						}
						pwm_set(servos[i].port_no,servos[i].pos);
						printf("\nServo #%d Pos = %d\n",
							servos[i].channel_num,servos[i].pos);
						break;
					case STATE_ERROR:
						printf("\nServo #%d has encountered an error - PC==%d\n",
							servos[i].channel_num,servos[i].pc);
					case STATE_END:
						printf("\nServo #%d: Execution has ended\n",
							servos[i].channel_num);
						break;
				}
			}
			printf("\n");
		}
	}

	//join servo thread and pwm threads
	pthread_join(run_thread, NULL);
	pthread_join(pwm_thread1, NULL);
	pthread_join(pwm_thread2, NULL);
	
	return 0;
}
