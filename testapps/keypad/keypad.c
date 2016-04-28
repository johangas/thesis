/*

Test application for the Contiki implementation for nRF52832

Simple keypad model which communicates with a security authority, opening doors only for authorized keys.

Written by Johan Gasslander, master thesis worker at SICS ICT Stockholm

*/

//Imports
#include <stdio.h>
#include <inttypes.h>
#include "clock.h"
#include "contiki.h"
#include "er-coap-engine.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "uip.h"
#include "er-coap-observe-client.h"

//Defines
#define COAP_PORT 	UIP_HTONS(COAP_DEFAULT_PORT)
#define REMOTE_PORT     UIP_HTONS(COAP_DEFAULT_PORT)
#define DOOR_OBS_URI "doors/door"

//Process handling
PROCESS(keypad_process, "keypad process");
PROCESS(door_process, "door process");

static uip_ipaddr_t authority_addr[1];
unsigned char led = 0;
long starttime = 0;
long time = 0;

AUTOSTART_PROCESSES(
	&door_process,
	&keypad_process
);

//TODO: increase accuracy
void starttimer(){
	//starttime = clock_seconds();
	starttime = (long) clock_time();
	printf("starttime: %ld \n", starttime);
}

void stoptimer(){
	//time = clock_seconds() - starttime;
	time = ((long) clock_time()) - starttime;
	printf("stoptime: %ld \n", time + starttime);
}

static void response(void *response){
	//TODO: what is this do
	const uint8_t *payload = NULL;
	coap_get_payload(response, &payload);
	stoptimer();
	//stop performance counter
	if(*payload != '0')
		leds_set(*payload);
	else {
		leds_set(0);
	}
	//print round trip time	
	printf("Notification from server received: %d Response time was %ld ms.\n", *payload, time);
}

static void callback(coap_observee_t *obst, void *notification, coap_notification_flag_t flag){
	//if authorized, shine for three seconds
	//if not authorised, blink for two seconds
	if (flag == NOTIFICATION_OK || flag == OBSERVE_OK) {
		if (notification){
			const uint8_t *payload = NULL;
			coap_get_payload(notification, &payload);
			stoptimer();
			//stop performance counter
			if(*payload != '0')
				leds_set(*payload);
			else {
				leds_set(0);
			}
			//print round trip time	
			printf("Notification from server received: %d Response time was %ld ms.\n", *payload, time);
			
		}
	}
}

/*void post_to_door(int value){
	static coap_packet_t request[1];
			coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
			coap_set_header_uri_path(request, "/doors/door");
			char str[4];
			sprintf(str, "%d", value);
			coap_set_payload(request, (uint8_t *) str, sizeof(str) - 1);
			COAP_BLOCKING_REQUEST(&authority_addr[0], REMOTE_PORT, request, response);
}*/

PROCESS_THREAD(door_process, ev, data){
	PROCESS_BEGIN();
	uiplib_ipaddrconv(ADDR, authority_addr);
	printf("Server addr: %s\n", ADDR);
	coap_init_engine();
	SENSORS_ACTIVATE(button_3);
	//clock_init();
	while(1){
		PROCESS_WAIT_EVENT();
		//observe authority and open door (set LED)
		if (data == &button_3 && button_3.value(BUTTON_SENSOR_VALUE_STATE) == 0){
			coap_obs_request_registration(authority_addr, COAP_PORT, DOOR_OBS_URI, callback, NULL);
			printf("Observing...");
		}
	}

	PROCESS_END();
}


PROCESS_THREAD(keypad_process, ev, data){
	PROCESS_BEGIN();
	SENSORS_ACTIVATE(button_1);	//auth door 1
  	SENSORS_ACTIVATE(button_2);	//not auth door 1
	
	while(1){
		PROCESS_WAIT_EVENT();
		//different button send different codes
		if (data == &button_1 && button_1.value(BUTTON_SENSOR_VALUE_STATE) == 0) {			
			//POST auth1
			printf("Button 1\n");
			starttimer();
			//post_to_door(4711);
			static coap_packet_t request[1];
			coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
			coap_set_header_uri_path(request, "/doors/door");
			coap_set_payload(request, (uint8_t *) "4711", 4);
			COAP_BLOCKING_REQUEST(&authority_addr[0], REMOTE_PORT, request, response);
			//start performance counter
			
		}
		if (data == &button_2 && button_2.value(BUTTON_SENSOR_VALUE_STATE) == 0) {
			//POST auth2 - not authorized
			printf("Button 2\n");
			starttimer();
			//post_to_door(3342);
			static coap_packet_t request[1];
			coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
			coap_set_header_uri_path(request, "/doors/door");
			coap_set_payload(request, (uint8_t *) "1234", 4);
			COAP_BLOCKING_REQUEST(&authority_addr[0], REMOTE_PORT, request, response);
			//start performance counter

		}
		
	}

	PROCESS_END();
}

