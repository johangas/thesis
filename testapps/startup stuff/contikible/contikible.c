/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/**
@file
Modification by Johan Gasslander johangas@kth.se

Test application for the Contiki implementation for nRF52832

*/

//Imports
#include "app_error.h"
#include "app_timer_appsh.h"
#include "ble_lbs.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_lbs.h"
#include "bsp.h"
#include "softdevice_handler.h"

#include "contiki.h"


#include <inttypes.h>
#include <stdio.h>
//Process handling
PROCESS(blink_process, "Blink process");
AUTOSTART_PROCESSES(&blink_process);

//Variables
static ble_lbs_t			m_lbs;
static ble_gap_sec_params_t             m_sec_params;
static uint16_t                         m_conn_handle = BLE_CONN_HANDLE_INVALID;
#define APP_TIMER_PRESCALER             0
#define APP_ADV_INTERVAL                64
#define APP_ADV_TIMEOUT_IN_SECONDS      180
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)
#define IS_SRVC_CHANGED_CHARACT_PRESENT 1
#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(500, UNIT_1_25_MS)
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(1000, UNIT_1_25_MS)
#define SLAVE_LATENCY                   0
#define WAKEUP_BUTTON_ID                0
//Help functions 

/*static void sleep_mode_enter(void)
{
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup button.
    err_code = bsp_wakeup_buttons_set((1 << WAKEUP_BUTTON_ID));
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}*/

static void bsp_event_handler(bsp_event_t evt)
{
	uint32_t err_code;
	static bool button_state = false;
		
	switch (evt)
	{
		// Handle any other event
		case BSP_EVENT_KEY_0:
			if (button_state)
			{
				err_code = ble_lbs_on_button_change(&m_lbs, 0);
			}
			else
			{
				err_code = ble_lbs_on_button_change(&m_lbs, 1);
			}
			button_state = !button_state;

			if (err_code != NRF_SUCCESS && err_code != BLE_ERROR_INVALID_CONN_HANDLE && err_code != NRF_ERROR_INVALID_STATE)
			{
				APP_ERROR_CHECK(err_code);
			}
			break;

		default:
			// APP_ERROR_HANDLER(evt);
			break;
	}
}
static void bsp_module_init(void)
{
    uint32_t err_code;
    // Note: If the buttons will be used to do some task, assign bsp_event_handler, as shown below.
    err_code = bsp_init(BSP_INIT_LED | BSP_INIT_BUTTONS, APP_TIMER_TICKS(100, APP_TIMER_PRESCALER), bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    // You can (if you configured an event handler) choose to assign events to buttons beyond the default configuration.
    // E.g:
    // err_code = bsp_event_to_button_action_assign(0, BSP_BUTTON_ACTION_RELEASE, BSP_EVENT_NOTHING);
    // APP_ERROR_CHECK(err_code);
}


static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t                         err_code;
    static ble_gap_evt_auth_status_t m_auth_status;
    bool                             master_id_matches;
    ble_gap_sec_kdist_t *            p_distributed_keys;
    ble_gap_enc_info_t *             p_enc_info;
    ble_gap_irk_t *                  p_id_info;
    ble_gap_sign_info_t *            p_sign_info;

    static ble_gap_enc_key_t         m_enc_key;           //< Encryption Key (Encryption Info and Master ID).
    static ble_gap_id_key_t          m_id_key;            //< Identity Key (IRK and address). 
    static ble_gap_sign_info_t       m_sign_key;          //< Signing Key (Connection Signature Resolving Key). 
    static ble_gap_sec_keyset_t      m_keys = {.keys_periph = {&m_enc_key, &m_id_key, &m_sign_key}};

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

            /* YOUR_JOB: Uncomment this part if you are using the app_button module to handle button
                         events (assuming that the button events are only needed in connected
                         state). If this is uncommented out here,
                            1. Make sure that app_button_disable() is called when handling
                               BLE_GAP_EVT_DISCONNECTED below.
                            2. Make sure the app_button module is initialized.
            err_code = app_button_enable();
            APP_ERROR_CHECK(err_code);
            */
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle = BLE_CONN_HANDLE_INVALID;

            /* YOUR_JOB: Uncomment this part if you are using the app_button module to handle button
                         events. This should be done to save power when not connected
                         to a peer.
            err_code = app_button_disable();
            APP_ERROR_CHECK(err_code);
            */
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle,
                                                   BLE_GAP_SEC_STATUS_SUCCESS,
                                                   &m_sec_params,
                                                   &m_keys);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle,
                                                 NULL,
                                                 0,
                                                 BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS | BLE_GATTS_SYS_ATTR_FLAG_USR_SRVCS);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_AUTH_STATUS:
            m_auth_status = p_ble_evt->evt.gap_evt.params.auth_status;
            break;

        case BLE_GAP_EVT_SEC_INFO_REQUEST:
            master_id_matches  = memcmp(&p_ble_evt->evt.gap_evt.params.sec_info_request.master_id,
                                        &m_enc_key.master_id,
                                        sizeof(ble_gap_master_id_t)) == 0;
            p_distributed_keys = &m_auth_status.kdist_periph;

            p_enc_info  = (p_distributed_keys->enc  && master_id_matches) ? &m_enc_key.enc_info : NULL;
            p_id_info   = (p_distributed_keys->id   && master_id_matches) ? &m_id_key.id_info   : NULL;
            p_sign_info = (p_distributed_keys->sign && master_id_matches) ? &m_sign_key         : NULL;

            err_code = sd_ble_gap_sec_info_reply(m_conn_handle, p_enc_info, p_id_info, p_sign_info);
                APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    //uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            //err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            //APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_IDLE:
            //sleep_mode_enter();
            break;

        default:
            break;	
    }
}

static void advertising_init(void)
{
	uint32_t      err_code;
	ble_advdata_t advdata;
	ble_advdata_t scanrsp;
	
	// YOUR_JOB: Use UUIDs for service(s) used in your application.
	ble_uuid_t adv_uuids[] = {{LBS_UUID_SERVICE, m_lbs.uuid_type}};

	// Build advertising data struct to pass into @ref ble_advertising_init.
	memset(&advdata, 0, sizeof(advdata));

	advdata.name_type               = BLE_ADVDATA_FULL_NAME;
	advdata.include_appearance      = true;
	advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
		
	memset(&scanrsp, 0, sizeof(scanrsp));
	scanrsp.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
	scanrsp.uuids_complete.p_uuids = adv_uuids;

	ble_adv_modes_config_t options = {0};
	options.ble_adv_fast_enabled  = BLE_ADV_FAST_ENABLED;
	options.ble_adv_fast_interval = APP_ADV_INTERVAL;
	options.ble_adv_fast_timeout  = APP_ADV_TIMEOUT_IN_SECONDS;

    	err_code = ble_advertising_init(&advdata, &scanrsp, &options, on_adv_evt, NULL);
    	APP_ERROR_CHECK(err_code);
}

static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    /* YOUR_JOB: Use an appearance value matching the application's use case.
    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_);
    APP_ERROR_CHECK(err_code); */

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
	on_ble_evt(p_ble_evt);
	ble_conn_params_on_ble_evt(p_ble_evt);
	ble_advertising_on_ble_evt(p_ble_evt);

	/*
	YOUR_JOB: Add service ble_evt handlers calls here, like, for example:
	ble_bas_on_ble_evt(&m_bas, p_ble_evt);
	*/
	ble_lbs_on_ble_evt(&m_lbs, p_ble_evt);
}

static void sys_evt_dispatch(uint32_t sys_evt)
{
    ble_advertising_on_sys_evt(sys_evt);
}

static void ble_stack_init(void)
{
    uint32_t err_code;

    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, NULL);

#if defined(S110) || defined(S130) || defined(S132)
    // Enable BLE stack 
    ble_enable_params_t ble_enable_params;
    memset(&ble_enable_params, 0, sizeof(ble_enable_params));
#if (defined(S130) || defined(S132))
    ble_enable_params.gatts_enable_params.attr_tab_size   = BLE_GATTS_ATTR_TAB_SIZE_DEFAULT;
#endif
    ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
    err_code = sd_ble_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);
#endif
    
    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);
    
    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}



//Main protothread 
PROCESS_THREAD(blink_process, ev, data){
	static struct etimer et_blink;

uint32_t err_code;
	PROCESS_BEGIN();
	advertising_init();
    	gap_params_init();
    	bsp_module_init();
   	ble_stack_init();
    	/*scheduler_init();

	services_init();
    	
    	conn_params_init();
    	sec_params_init();*/
	
    // Start execution
    	err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    	APP_ERROR_CHECK(err_code);
	printf("Prime started...\n");
	etimer_set(&et_blink, CLOCK_SECOND/100);
 
	while(1){
		//app_sched_execute();
        	//power_manage();
	}

	PROCESS_END();
}

