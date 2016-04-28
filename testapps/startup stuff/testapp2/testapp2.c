/*

Test application for the Contiki implementation for nRF52832
prints primes to console with 2s intervals

*/

//Imports
#include <stdio.h>
#include <inttypes.h>
#include "contiki.h"
#include "button-sensor.h"
#include "dev/leds.h"

//Process handling
PROCESS(led_process, "led process");
PROCESS(button_process, "button process");

const struct sensors_sensor *button1;
unsigned char led = 0;

AUTOSTART_PROCESSES(
	&button_process,
	&led_process
);



//Shifts a variable and sets LEDs
PROCESS_THREAD(led_process, ev, data){
	static struct etimer et_led;
	PROCESS_BEGIN();
	printf("LED process\r\n");
	etimer_set(&et_led, CLOCK_SECOND);
	leds_init();
	leds_set(0);
	while(1){
		PROCESS_WAIT_EVENT();
		if (ev == PROCESS_EVENT_TIMER && etimer_expired(&et_led) ){
			led = (led << 1) % 16;
			leds_set(led);
			printf("Shifting\r\n");
			etimer_reset(&et_led);
		}
	}

	PROCESS_END();
}

//Reads button presses and adds to variable for LEDs
PROCESS_THREAD(button_process, ev, data){
	PROCESS_BEGIN();
	printf("button process\r\n");
	button1 = &button_1;
	button1->configure(SENSORS_ACTIVE, 1);
        led = 1;
	while(1){
		PROCESS_WAIT_EVENT();
		if (ev == sensors_event && data == button1) {
			led = led | 1;
			leds_on(led);
			printf("Button pressed\r\n");
		}
	}

	PROCESS_END();
}

