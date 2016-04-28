

#include "contiki.h"
#undef MIN
#undef MAX
#include "softdevice_handler.h"
#include "nordic_common.h"
#include "stdint.h"
#include "dev/leds.h"
#include "nrf_soc.h"
#include "ble/ble-core.h"
#include "ble/ble-mac.h"
//#include "ble-ipsp.h"
#include <string.h>
#include <stdio.h> /* For printf() */
#include <psock.h>

#include "ble_srv_common.h"
#include "ble_advdata.h"

/*---------------------------------------------------------------------------*/
#define ADV_INTERVAL_IN_MS              1200
#define ADV_INTERVAL                    MSEC_TO_UNITS(ADV_INTERVAL_IN_MS, UNIT_0_625_MS) /**< The advertising interval (in units of 0.625 ms. */
#define ADV_TIMEOUT_IN_SECONDS          0       /**< The advertising timeout (in units of seconds). */

/*---------------------------------------------------------------------------*/
//extern void ble_gap_addr_print(const ble_gap_addr_t *addr);
/*---------------------------------------------------------------------------*/
static void sys_evt_dispatch(uint32_t sys_evt);
static void ble_evt_dispatch(ble_evt_t * p_ble_evt);
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name);
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name);

/*---------------------------------------------------------------------------*/
static struct etimer et_hello;
static struct etimer et_blink;
static uint8_t blinks;
//static struct psock socket;
//static uint8_t buffer[100];

/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
PROCESS(blink_process, "LED blink process");
AUTOSTART_PROCESSES(&hello_world_process, &blink_process);

/*---------------------------------------------------------------------------
static void ble_evt_dispatch(ble_evt_t * p_ble_evt) {
	switch(p_ble_evt->header.evt_id) {
    		case BLE_GAP_EVT_CONNECTED:
      			printf("ble-core: connected [handle:%d, peer: ", p_ble_evt->evt.gap_evt.conn_handle);
      			ble_gap_addr_print(&(p_ble_evt->evt.gap_evt.params.connected.peer_addr));
      			printf("]\n");
      			sd_ble_gap_rssi_start(p_ble_evt->evt.gap_evt.conn_handle,
                            BLE_GAP_RSSI_THRESHOLD_INVALID,
                            0);
      			break;

    		case BLE_GAP_EVT_DISCONNECTED:
      			printf("ble-core: disconnected [handle:%d]\n", p_ble_evt->evt.gap_evt.conn_handle);
      			ble_advertising_start();
      			break;
    		default:
			printf("ble-core: nothing to do");
      			break;
  	}
}*/

static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
}
/*---------------------------------------------------------------------------*/
static void sys_evt_dispatch(uint32_t sys_evt)
{
}
/*---------------------------------------------------------------------------
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name) {
	printf("Error of %X at %d in %s\n",(unsigned int) error_code, (int)line_num, p_file_name);
	//while(1){};
}*/
/*
static int handle_connection(struct psock *p){
	PSOCK_BEGIN(p);
	PSOCK_SEND_STR(&socket, "GET / HTTP/1.0\r\n");
	printf("sent DEADBEEF");
	PSOCK_END(p);
}*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD (hello_world_process, ev, data) {
	
	PROCESS_BEGIN();
	/*uint32_t                err_code;
	SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, false);
	err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
  	APP_ERROR_CHECK(err_code);
  	
  	err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
  	APP_ERROR_CHECK(err_code);
	ble_gap_conn_sec_mode_t sec_mode;
	char name_buffer[9];
	sprintf(name_buffer, "Nordic_Blinky");
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
 	err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)name_buffer,
        	strlen(name_buffer));
//	ble_advdata_t advdata;
	printf("starting");
	uip_ipaddr_t addr;

	uip_ipaddr(&addr, 192,10,66,197);
  	tcp_connect(&addr, UIP_HTONS(80), NULL);
	if(uip_aborted() || uip_timedout() || uip_closed()) {
    		printf("Could not establish connection\n");
  	}
	//	PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
  	//printf("ble init\n");
	//ble_stack_init();
  	//printf("ble adv\n");
	//ble_advertising_init("Nordic_Blinky");
	//uint8_t mac = print_local_addresses();
	//printf("MAC: %x\n", mac);
	//linkaddr_t addr;
	//ble_ipsp_handle_t* p_handle = find_handle(&addr);
	//ble_ipsp_connect(p_handle);
	  ble_advdata_service_data_t service_data[2];
	  uint8_t battery_data = 98;//battery_level_get();
	  uint32_t temperature_data = 0xFE001213;
	  service_data[0].service_uuid = BLE_UUID_BATTERY_SERVICE;
	  service_data[0].data.size    = sizeof(battery_data);
	  service_data[0].data.p_data  = &battery_data;
	  service_data[1].service_uuid = BLE_UUID_HEALTH_THERMOMETER_SERVICE;
	  service_data[1].data.size    = sizeof(temperature_data);
	  service_data[1].data.p_data  = (uint8_t *) &temperature_data;
	  // Build and set advertising data
	  memset(&advdata, 0, sizeof(advdata));
	  advdata.name_type            = BLE_ADVDATA_FULL_NAME;
	  advdata.include_appearance   = false;
	  advdata.service_data_count   = 2;
	  advdata.p_service_data_array = service_data;
	  err_code = ble_advdata_set(&advdata, NULL);
	  APP_ERROR_CHECK(err_code);

	  ble_gap_adv_params_t adv_params;
	  // Start advertising
	  memset(&adv_params, 0, sizeof(adv_params));
	  adv_params.type        = BLE_GAP_ADV_TYPE_ADV_NONCONN_IND;
	  adv_params.p_peer_addr = NULL;
	  adv_params.fp          = BLE_GAP_ADV_FP_ANY;
	  adv_params.interval    = ADV_INTERVAL;
	  adv_params.timeout     = ADV_TIMEOUT_IN_SECONDS;
	  err_code = sd_ble_gap_adv_start(&adv_params);
	  APP_ERROR_CHECK(err_code);
	  leds_off(LEDS_ALL);
	  leds_on(LEDS_RED);
  	etimer_set(&et_hello, CLOCK_SECOND*2);
*/
	
	
  	while(1) {
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
		
		/*if(uip_aborted() || uip_timedout() || uip_closed()) {
	    		printf("Could not establish connection\n");
	  	}
		if(uip_connected()) {
			printf("Connected\n");
			PSOCK_INIT(&socket, buffer, sizeof(buffer));	
			handle_connection(&socket);
			//sd_rand_application_vector_get(&blinks, 1);
			//printf("Sensor says #%X\n", (unsigned int) blinks);
			//process_post(&ipsp_process, PROCESS_EVENT_CONTINUE, blinks);
			etimer_reset(&et_hello);
		} else  {
			printf("Not connected...");
		}*/
		printf("alive");
		etimer_reset(&et_hello);
  		

	}
  	PROCESS_END();
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(blink_process, ev, data) {
  	PROCESS_BEGIN();

  	blinks = 0;
  	while(1) {
		etimer_set(&et_blink, CLOCK_SECOND*5);
		//EVENT MSG
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
		//printf("data?%x", data);
		leds_off(LEDS_ALL);
		leds_on(blinks & LEDS_ALL);
		printf("Blink... (state %x)", leds_get());
		etimer_reset(&et_blink);
  	}

  	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
