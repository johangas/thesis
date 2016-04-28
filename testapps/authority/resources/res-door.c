#include <string.h>
#include "stdlib.h"
#include "contiki.h"
#include "rest-engine.h"
#include "dev/leds.h"
#include "codes.h"

static void res_post_put_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

static void res_event_handler();

EVENT_RESOURCE(res_door, "title=\"door\"; obs", res_get_handler, res_post_put_handler, res_post_put_handler, NULL, res_event_handler);

int strcompare(char *str, const uint8_t *payload) {
//	char pay[] = payload

	/*FIXME if(sizeof(str) != sizeof(payload)){
		printf("Keys differ in size, %d, %d\n", sizeof(str), sizeof(payload));
		return 0;
	}*/ 
	//printf("Same size: %d\n", sizeof(str));
	printf("%d, %d", *str, *payload);
	while(*str == *payload) {
		if(*str == '\0' || *payload == '\0')
			break;
		
		str++;
		payload++;
	}
	if(*str == '\0' && *payload == '\0'){
		return 1;
	} else {
		return 0;
	}
}


static void res_post_put_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
	//Look if authorized and then set led1
	const uint8_t *payload;
  	REST.get_request_payload(request, &payload);
	char str[4];
	for(int i = 0; i < codesize; i++){
		printf("%s, %d\n", payload, allowed[i]);
		sprintf(str, "%d", allowed[i]);
		
		if(strcompare(str, payload)){
			printf("Authorized, opening door\n");
			leds_set(1);
			REST.notify_subscribers(&res_door);
			REST.set_response_status(response, REST.status.CHANGED);
			return;
		} else {
			printf("Declined, door closed\n");
			leds_set(0);
		}
		
	}
	REST.notify_subscribers(&res_door);
	REST.set_response_status(response, REST.status.CHANGED);
}

static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset){
	//get must be safe so only return value of the door
	REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
	REST.set_response_payload(response, buffer, snprintf((char *)buffer, preferred_size, "%d", leds_get()));
}

static void res_event_handler() {
	REST.notify_subscribers(&res_door);
}
