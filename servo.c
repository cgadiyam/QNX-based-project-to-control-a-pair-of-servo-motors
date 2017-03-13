/* 

Real-Time Operating Systems
Project 2b
Chaithanya Gadiyam & Michael Moffitt
*/

#include <stdlib.h>
#include "servo.h" 

/*

#define STATE_BEGIN 0
#define STATE_RUN 1
#define STATE_PAUSE 2
#define STATE_END 3
#define STATE_ERROR 4

*/

void servo_set_recipe(servo *s, u8 *r)
{
 	s->recipe = r;
}

void servo_print_state(servo *s)
{
	printf("\nServo at %X ----------|\n",(u32)s);
	printf("  pwm_val: %X\n",(u32)s->pwm_val);
	printf("  pc: %X\n",(u16)s->pc);
	printf("  wait: %X\n",(u16)s->wait);
	printf("  loop_ptr: %X\n",(u16)s->loop_ptr);
	printf("  pos: %X\n",(u8) s->pos);
	printf("  loop_num: %X\n",(u8) s->loop_num);
	printf("  err: %X\n",(u8) s->err);
	printf("  finished: %X\n",(u8) s->finished);
	printf("  channel_num: %X\n",(u8) s->channel_num);
	printf("  recipe: %X\n",(u8) s->recipe);
	printf("-----------------------|\n");
}

void servo_init(servo *s, u8 timer)
{
    s->pwm_val = 0;
	s->pc = 0;
	s->wait = 0;
	s->loop_ptr = 0;
	s->pos = 0;
	s->loop_num = 0;
	s->err = 0;
	s->finished = 0;
	s->errno = 0;
	s->state = STATE_PAUSE;
	
	// Tell the servos to wait a bit so they won't be in the wrong position
	//s->wait = MOVE_DELAY_CYCLES * SERVO_MAX_POS;
	s->wait = 0;
	s->channel_num = timer + 2;
	s->port_no = timer;
}

void servo_process_opcode(servo* s, u8 prog_data)
{	
	u8 instruction = prog_data;
	u8 opcode = get_opcode(instruction); 
	u8 param = get_param(instruction);  
	switch (opcode)
	{
		case OP_MOV:
		 	//(void)printf("\n\r#%d: MOV %X",s->channel_num,param);  
			// Out of range, set error flag
			if (param > SERVO_MAX_POS)
			{
				//(void)printf("ERR: Servo on channel %d move out of range: %d\n\r",s->channel_num,param);
				s->err = 1;	   
				s->errno = ERR_RECIPE;
				return;
			}
			
			
			s->wait += (MOVE_DELAY_CYCLES * ((s->pos > param) 
				? (s->pos - param) 
				: (param - s->pos)));
				
			//(void)printf(", waiting for %d cycles",s->wait);
			
			// Store new position
			s->pos = param;
			pwm_set(s->port_no,s->pos);
			s->pc++;
			break;
		// Add to delay variable
		case OP_WAIT:	   
		//(void)printf("\n\r#%d: WAIT %X",s->channel_num,param);
			s->wait += param * MOVE_DELAY_CYCLES; 
			s->pc++;
			break;
			
		// Mark beginning of loop, record pc+1
		case OP_LOOP_START:	 
			//(void)printf("\n\r#%d: LOOP %X",s->channel_num,param);
			// No nested loops, error out if it occurs
			if (s->loop_num > 0)
			{			 
				//(void)printf("\n\rERR: Servo on channel %d has illegal nested loop",s->channel_num);
				s->err = 1;
				s->errno = ERR_LOOP;
				return;
			}
			s->loop_ptr = s->pc+1;
			s->loop_num = param;
			s->pc++;
			break;
			
		case OP_MOVC:
			//(void)printf("\n\r#%d: MOVC %X",s->channel_num,param);  
			// Out of range, set error flag
			if (param > SERVO_MAX_POS)
			{
				//(void)printf("\n\rERR: %d out of range\n\r",s->channel_num);
				s->err = 1;					 
				s->errno = ERR_RECIPE;
				return;
			}
			
			
			if((s->pos + param) <= SERVO_MAX_POS)
			{
			
				s->wait += (MOVE_DELAY_CYCLES * param);
					
				//(void)printf(", waiting for %d cycles",s->wait);
				
				s->pos += param;
			
			}
			else
			{
				s->wait += (MOVE_DELAY_CYCLES * (SERVO_MAX_POS - s->pos));
					
				//(void)printf(", waiting for %d cycles",s->wait);
				
				s->pos = SERVO_MAX_POS;
			}
			
			// Store new position
			s->pos = param;
			pwm_set(s->port_no,s->pos);
			s->pc++;
			break;
				   
		// Jump to stored address if there are >0 loops to do
		case OP_END_LOOP:  
			//(void)printf("\n\r#%d: ENDLOOP",s->channel_num);
			if (s->loop_num > 0)
			{
				s->pc = s->loop_ptr;
				s->loop_num--;
			}
			else
			{
			 	s->pc++;
			}
			break;
			
		// End of program
		case OP_RECIPE_END:			
			//(void)printf("\n\r#%d: END",s->channel_num);
			s->finished = 1;
			break;
			
		case OP_ERR2:
		case OP_ERR3:
		// Other opcodes will raise an error  
			//(void)printf("\r\nERR: %d illegal",s->channel_num);
			s->err = 1;
			s->errno = ERR_RECIPE;
			break;
	}
}			

// Runs every ISR (every 0.1s)
void servo_kernel(servo* s) 
{		 
	// Don't execute if there's an error
	if (s->err || s->finished || s->state != STATE_RUN)
	{
		if (s->err)
		{
			s->state = STATE_ERROR;
		}
		else if (s->finished)
		{
			s->state = STATE_END; // "Recipe_end"
		}
		return;
	}
			   
	// No waiting to do, execute next instruction
	if (s->wait == 0)
	{
		servo_process_opcode(s, *(u8 *)(s->recipe + s->pc));
	}
	
	// Wait for one "cycle"
	else
	{
		//(void)printf(".");
		s->wait--;
	}
	
}

// Global servo state;
servo *global_servos;
