/*

Test application for the Contiki implementation for nRF52832
ICMP

*/

//Imports
#include <stdio.h>
#include <inttypes.h>
#include "contiki.h"
#include "button-sensor.h"
#include "dev/leds.h"

//Process handling
PROCESS(echo_process, "echo process");
PROCESS(send_process, "send process");

const struct sensors_sensor *button1;


AUTOSTART_PROCESSES(
	&send_process,
	&echo_process
);


//Shifts a variable and sets LEDs
PROCESS_THREAD(echo_process, ev, data){
	
	PROCESS_BEGIN();
	
	while(1){
		PROCESS_WAIT_EVENT(); //RCV ICMP?
		//blink
		//echo
	}

	PROCESS_END();
}

//Reads button presses and adds to variable for LEDs
PROCESS_THREAD(send_process, ev, data){
	PROCESS_BEGIN();
	
	while(1){
		PROCESS_WAIT_EVENT(); //button
		//SEND ICMP
		//wait for reply
		//blink
	}

	PROCESS_END();
}

