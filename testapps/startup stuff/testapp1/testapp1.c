/*

Test application for the Contiki implementation for nRF52832
prints primes to console with 2s intervals

*/

//Imports
#include <stdio.h>
#include <inttypes.h>
#include "contiki.h"

//Process handling
PROCESS(prime_process, "Prime process");
PROCESS(print_process, "Print process");

AUTOSTART_PROCESSES(
	&print_process,
	&prime_process
);

//Variables
int prime = 0, i = 3, c;

//Prime calculating process
PROCESS_THREAD(prime_process, ev, data){
	static struct etimer et_prime;
	PROCESS_BEGIN();
	printf("Prime started...\n");
	etimer_set(&et_prime, CLOCK_SECOND/100);
	while(1){
		
		for ( c = 2 ; c <= i - 1 ; c++ ) {
       			if ( i%c == 0 )
				break;
		}

		if ( c == i ) {
			//printf("Prime: Prime found. %d\n", i);
			prime = i;
			i++;
			PROCESS_WAIT_EVENT(); //RESETS variables?
			etimer_reset(&et_prime);
		} else {
			i++;
		}
		

	}

	PROCESS_END();
}

//Timer controlled printing process
PROCESS_THREAD(print_process, ev, data){

	static struct etimer et_print;
	PROCESS_BEGIN();
	printf("Print started...\n");
	etimer_set(&et_print, CLOCK_SECOND);
	printf("Print: Timer started.\n");

	while(1){
		PROCESS_WAIT_EVENT();
			
		if(ev == PROCESS_EVENT_TIMER && etimer_expired(&et_print)){
			printf("Prime: %d\n", prime);
			etimer_reset(&et_print);
		}		
	}

	PROCESS_END();
}
	
