/* 

Real-Time Operating Systems
Project 2b
Chaithanya Gadiyam & Michael Moffitt
*/

#include "opcode.h"

u8 get_opcode(u8 byte)
{
	return byte >> 5;
}

u8 get_param(u8 byte)
{
	return byte & 0x1F;
}