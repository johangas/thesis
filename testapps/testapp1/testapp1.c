/*

Test application for the Contiki implementation for nRF52dk


*/

//Imports
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include <er-coap-engine.h>
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "uip.h"
#include "er-coap-observe-client.h"

//Defines
#define COAP_PORT       UIP_HTONS(COAP_DEFAULT_PORT)
#define OBS_RESOURCE_URI  "lights/led3"
#define PLATFORM_INDICATE_BLE_STATE   1

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
#ifndef ADDR
#define ADDR 0
#endif

//Process handling
PROCESS(client_process, "Client process");
PROCESS(server_process, "Server process");

AUTOSTART_PROCESSES(
	&client_process,
	&server_process
);

//Variables
extern resource_t res_led3; //this resource
uint8_t obs_led = 1;	//the specific leds to be observed for this instance
static uip_ipaddr_t server_ipaddr[4]; //ip-adresses of other clients
static coap_observee_t *obs[4];
int obscount = 0;

static void
notification_callback(coap_observee_t *obst, void *notification,
                      coap_notification_flag_t flag) {
	int len = 0;
	const uint8_t *payload = NULL;

	if (notification) {
    		len = coap_get_payload(notification, &payload);
  	}
	/*int id = 0;
	for(int i = 0; i < 4; i++)
		if(obst == obs[i])
			id = i;
	*/
	//what is reply
	switch(flag){
		case NOTIFICATION_OK:
			if(*payload == '1') {
				obs_led++;
			} else {
				if(obs_led > 0)
					obs_led--;
			}
		break;
		case OBSERVE_OK: //first time only
			if(*payload == '1') {
				obs_led++;
			} else {
				if(obs_led > 0)
					obs_led--;
			}
		break;
		case OBSERVE_NOT_SUPPORTED:
			PRINTF("OBSERVE_NOT_SUPPORTED: %*s\n", len, (char *)payload);
			obst = NULL;
		break;
		case ERROR_RESPONSE_CODE:
			PRINTF("ERROR_RESPONSE_CODE: %*s\n", len, (char *)payload);
			obst = NULL;
		break;
		case NO_REPLY_FROM_SERVER:
			PRINTF("NO_REPLY_FROM_SERVER: removing observe registration with token %x%x\n",
				obst->token[0], obst->token[1]);
			obst = NULL;
		break;
	}
	
	//set led accordingly
	PRINTF("Observer leds: %d\n", obs_led);
	//leds_set((leds_get() & 0x8) | obs_led);
	leds_set(obs_led);
}

//Observes other servers
PROCESS_THREAD(client_process, ev, data){
	PROCESS_BEGIN();
	
	
	uiplib_ipaddrconv(ADDR, server_ipaddr);	//Handle IPs for other devices
  	//coap_init_engine();
	
  	SENSORS_ACTIVATE(button_1);	//observe all
  	SENSORS_ACTIVATE(button_2);	//remove observers

	while(1){
		PROCESS_WAIT_EVENT(); 
		if (data == &button_1 && button_1.value(BUTTON_SENSOR_VALUE_STATE) == 0) {
        		PRINTF("Starting observation. Observers: %d \n", obscount);
			obscount++;
			for(int i = 0; i < obscount; i++)
        			obs[i] = coap_obs_request_registration(&server_ipaddr[i], COAP_PORT, OBS_RESOURCE_URI, notification_callback, NULL);
      		}
		if (data == &button_2 && button_2.value(BUTTON_SENSOR_VALUE_STATE) == 0) {
			obscount = 0;
        		PRINTF("Stopping observation. Observers: %d \n", obscount);
			for(int i = 0; i < obscount; i++) {
	        		coap_obs_remove_observee(obs[i]);
	        		obs[i] = NULL;
			}
      		}
		
	}

	PROCESS_END();
}

//Handles Erbium server
PROCESS_THREAD(server_process, ev, data){
	PROCESS_BEGIN();
	rest_init_engine();

  	SENSORS_ACTIVATE(button_3);//toggle led3
  	rest_activate_resource(&res_led3, "lights/led3");

	while(1){
		PROCESS_WAIT_EVENT();
		if (ev == sensors_event) {
			if (data == &button_3 && button_3.value(BUTTON_SENSOR_VALUE_STATE) == 0) {
				printf("Toggle LED 3\n");
        			leds_toggle(LEDS_3);
        			res_led3.trigger();
      			}
    		}
	}

	PROCESS_END();
}
	
