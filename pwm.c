/* 

Real-Time Operating Systems
Project 2b
Chaithanya Gadiyam & Michael Moffitt
*/

#include "pwm.h"
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <stdlib.h>

void pwm_init(u8 channel)
{
	u8 i;
	// Set sane defaults so the poor servos don't kill themselves
	for (i = 0; i < 8; i++)
	{
	 	pwm_set(i,0);
	}
}

//sets the pulse width based on servo position
void pwm_set(u8 port_no, u8 pos)
{
	int count[6]= {6,12,18,24,30,40};
	if(port_no == 0)
	{
		Pulse_Width1 = (count[pos]) * PWM_ADJ;
	}
	else if(port_no == 1)
	{
		Pulse_Width2 = (count[pos]) * PWM_ADJ;
	}
	else
	{
	}
}

//Thread routine for pwm thread 1
void *pwm_routine1(void *args)
{
	 int pid;
	    int chid;
	    int pulse_id = 0 ;
	    timer_t timer_id;
	    struct sigevent event;
	    struct itimerspec timer;
	    struct _pulse pulse;

	    // Create a channel to receive timer events on
	    chid = ChannelCreate( 0 );
	    // if returns a -1 for failure we stop with error
	    assert ( chid != -1 );

	    // Set up the timer and timer event.
	    event.sigev_notify = SIGEV_PULSE;
	    event.sigev_coid = ConnectAttach ( ND_LOCAL_NODE, 0, chid, 0, 0 );
	    assert ( event.sigev_coid != -1 );
	    event.sigev_priority = getprio(0);
	    event.sigev_code = 1023;
	    event.sigev_value.sival_ptr = (void*)pulse_id;

	    // Now create the timer and get back the timer_id value for the timer we created.
	    if ( timer_create( CLOCK_REALTIME, &event, &timer_id ) == -1 )
	    {
	        perror ( "can't create timer" );
	        exit( EXIT_FAILURE );
	    }

	    // Change the timer configuration to set its period interval.
	    // First Interrupt at 4 msec
	    // Keep interrupting every 7.2 msec
	    timer.it_value.tv_sec = 0;
	    timer.it_value.tv_nsec = 1000000; // 1ms
	    timer.it_interval.tv_sec = 0;
	    timer.it_interval.tv_nsec = 7200000;  // interrupt every 7.2 msec

	    // Start the timer
	    if ( timer_settime( timer_id, 0, &timer, NULL ) == -1 )
	    {
	        perror("Can't start timer.\n");
	        exit( EXIT_FAILURE );
	    }



	printf("pwm thread for servo 1 \n");
	int privity_err;
	/* Give this thread root permissions to access the hardware */
	privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
	if ( privity_err == -1 )
	{
		fprintf( stderr, "can't get root permissions\n" );
	}

	//configure port A as output port
	uintptr_t Ctrl_handle;
	Ctrl_handle = mmap_device_io(PORT_LENGTH, CTRL_ADDRESS);
	out8(Ctrl_handle, 0x00);

	unsigned int *pw = (unsigned int*)args;
	uintptr_t PortA_handle;
	PortA_handle = mmap_device_io(PORT_LENGTH, PORTA_ADDRESS);

	for (;;)
	{
		pid = MsgReceivePulse ( chid, &pulse, sizeof( pulse ), NULL );
		// Do high out8 on port A
		// -------
		out8( PortA_handle, 0xFF);
		nanospin_ns(*pw);
		// Now go low
		// -------
		out8( PortA_handle, 0x00);
	}
}

//Thread routine for pwm thread 2
void *pwm_routine2(void *args)
{
	 int pid;
	    int chid;
	    int pulse_id = 0 ;
	    timer_t timer_id;
	    struct sigevent event;
	    struct itimerspec timer;
	    struct _pulse pulse;

	    // Create a channel to receive timer events on
	    chid = ChannelCreate( 0 );
	    // if returns a -1 for failure we stop with error
	    assert ( chid != -1 );

	    // Set up the timer and timer event.
	    event.sigev_notify = SIGEV_PULSE;
	    event.sigev_coid = ConnectAttach ( ND_LOCAL_NODE, 0, chid, 0, 0 );
	    assert ( event.sigev_coid != -1 );
	    event.sigev_priority = getprio(0);
	    event.sigev_code = 1023;
	    event.sigev_value.sival_ptr = (void*)pulse_id;

	    // Now create the timer and get back the timer_id value for the timer we created.
	    if ( timer_create( CLOCK_REALTIME, &event, &timer_id ) == -1 )
	    {
	        perror ( "can't create timer" );
	        exit( EXIT_FAILURE );
	    }

	    // Change the timer configuration to set its period interval.
	    // First Interrupt at 4 msec
	    // Keep interrupting every 7.2 msec
	    timer.it_value.tv_sec = 0;
	    timer.it_value.tv_nsec = 4000000; // 4ms
	    timer.it_interval.tv_sec = 0;
	    timer.it_interval.tv_nsec = 7200000;  // interrupt every 7.2 msec

	    // Start the timer
	    if ( timer_settime( timer_id, 0, &timer, NULL ) == -1 )
	    {
	        perror("Can't start timer.\n");
	        exit( EXIT_FAILURE );
	    }

	printf("pwm thread for servo 2 \n");
	int privity_err;
	/* Give this thread root permissions to access the hardware */
	privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
	if ( privity_err == -1 )
	{
		fprintf( stderr, "can't get root permissions\n" );
	}

	//configure port B as output port
	uintptr_t Ctrl_handle;
	Ctrl_handle = mmap_device_io(PORT_LENGTH, CTRL_ADDRESS);
	out8(Ctrl_handle, 0x00);

	unsigned int *pw = (unsigned int*)args;
	uintptr_t PortB_handle;
	PortB_handle = mmap_device_io(PORT_LENGTH, PORTB_ADDRESS);

	for (;;)
	{
		pid = MsgReceivePulse ( chid, &pulse, sizeof( pulse ), NULL );
		// Do high out8 on port A
		// -------
		out8( PortB_handle, 0xFF);
		nanospin_ns(*pw);
		// Now go low
		// -------
		out8( PortB_handle, 0x00);
	}
}
