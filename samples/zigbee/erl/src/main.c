/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *
 * @brief Zigbee application template.
 */

#include <zephyr.h>
#include <logging/log.h>
#include <dk_buttons_and_leds.h>

#include <stdio.h>

#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include <zigbee/zigbee_app_utils.h>
#include <zb_nrf_platform.h>


/* ERL Device endpoint */
#define ERL_ENDPOINT                    12

/* Range Extender Endpoint */
#define APP_TEMPLATE_ENDPOINT			10

/* LED indicating that device successfully joined Zigbee network. */
#define ZIGBEE_NETWORK_STATE_LED        DK_LED3

/* LED used for device identification. */
#define IDENTIFY_LED                    DK_LED4

/* Button used to enter the Identify mode. */
#define IDENTIFY_MODE_BUTTON            DK_BTN4_MSK

#define ERL_ATTRIBUTE_REPORT_INTERVAL	(ZB_TIME_ONE_SECOND * 10)

LOG_MODULE_REGISTER(app);

struct
{
  zb_erl_basic_server_attrs_t basic_attrs;
  zb_erl_time_server_attrs_t time_attrs;
  zb_erl_metering_server_attrs_t metering_attrs;
  zb_erl_daily_schedule_server_attrs_t daily_schedule_attrs;
  zb_erl_meter_identification_server_attrs_t meter_identification_attrs;
  zb_erl_el_measurement_server_attrs_t el_measurement_attrs;
  zb_addr_u daily_schedule_client_address;
  zb_uint8_t daily_schedule_client_ep;
  zb_erl_identify_server_attrs_t identify_attrs;
} erl_data;

struct zb_device_ctx {
	zb_zcl_basic_attrs_t     basic_attr;
	zb_zcl_identify_attrs_t  identify_attr;
};

/* Zigbee device application context storage. */
static struct zb_device_ctx dev_ctx;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(
	identify_attr_list,
	&dev_ctx.identify_attr.identify_time);

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(
	basic_attr_list,
	&dev_ctx.basic_attr.zcl_version,
	&dev_ctx.basic_attr.power_source);

ZB_HA_DECLARE_RANGE_EXTENDER_CLUSTER_LIST(
	app_template_clusters,
	basic_attr_list,
	identify_attr_list);

ZB_HA_DECLARE_RANGE_EXTENDER_EP(
	app_template_ep,
	APP_TEMPLATE_ENDPOINT,
	app_template_clusters);

ZB_ERL_DECLARE_METER_IDENTIFICATION_ATTR_LIST(meter_attr, erl_data.meter_identification_attrs);
/* Basic cluster attributes */
ZB_ERL_DECLARE_BASIC_ATTR_LIST(erl_interface_basic_attr_list, erl_data.basic_attrs);
/* Time cluster attributes */
ZB_ERL_DECLARE_TIME_ATTR_LIST(erl_interface_time_attr_list, erl_data.time_attrs);
/* Identify cluster attributes */
ZB_ERL_DECLARE_IDENTIFY_ATTR_LIST(erl_interface_identify_attr_list, erl_data.identify_attrs);
/* Daily schedule cluster attributes */
ZB_ERL_DECLARE_DAILY_SCHEDULE_ATTR_LIST(erl_interface_daily_schedule_attr_list,
                                        erl_data.daily_schedule_attrs);
/* Meter identification cluster attributes */
ZB_ERL_DECLARE_METER_IDENTIFICATION_ATTR_LIST(erl_interface_meter_identification_attr_list,
                                              erl_data.meter_identification_attrs);
/* El Measurement cluster attributes */
ZB_ERL_DECLARE_EL_MEASUREMENT_ATTR_LIST(erl_interface_el_measurement_attr_list,
                                        erl_data.el_measurement_attrs);
/* Diagnostics cluster attributes */
ZB_ZCL_DECLARE_DIAGNOSTICS_ATTRIB_LIST(erl_interface_diagnostics_attr_list);
/* Metering cluster attributes */
ZB_ERL_DECLARE_METERING_ATTR_LIST(erl_interface_metering_attr_list, erl_data.metering_attrs);

/* Declare cluster list for a device */
ZB_DECLARE_ERL_INTERFACE_DEV_CLUSTER_LIST(erl_interface_dev_clusters,
                                          erl_interface_basic_attr_list,
                                          erl_interface_identify_attr_list,
                                          erl_interface_time_attr_list,
                                          erl_interface_meter_identification_attr_list,
                                          erl_interface_el_measurement_attr_list,
                                          erl_interface_diagnostics_attr_list,
                                          erl_interface_metering_attr_list,
                                          erl_interface_daily_schedule_attr_list);

/* Declare endpoint */
ZB_DECLARE_ERL_INTERFACE_DEV_EP(erl_interface_dev_ep,
                                ERL_ENDPOINT,
                                erl_interface_dev_clusters);

ZBOSS_DECLARE_DEVICE_CTX_2_EP(
	app_template_ctx,
	erl_interface_dev_ep,
	app_template_ep);


/**@brief Function for initializing all clusters attributes. */
static void app_clusters_attr_init(void)
{

}


// static void update_attrs(zb_uint8_t _)
// {
// 	ZVUNUSED(_);
// 	zb_zcl_reporting_info_t *erl_reporting_info;
// 	zb_char_t erl_pod_buf[16] = "";
// 	static zb_uint8_t erl_pod_idx;

// 	// Get a new Point Of Delivery
//     switch ((erl_pod_idx++) % 4)
//     {
//         case 0:
//             sprintf(erl_pod_buf, "abcdeabcdefg\0");
//             break;
//         case 1:
//             sprintf(erl_pod_buf, "abcdeabcdef\0");
//             break;
//         case 2:
//             sprintf(erl_pod_buf, "abcdeabcde\0");
//             break;
//         case 3:
//             sprintf(erl_pod_buf, "Dabcdefghi001\0");
//             break;
//     }

//     ZB_ZCL_STRING_CLEAR(erl_data.meter_identification_attrs.pod);
//     ZB_ZCL_STRING_APPEND_C_STR(erl_data.meter_identification_attrs.pod, 16, erl_pod_buf);

// 	erl_reporting_info = zb_zcl_find_reporting_info(ERL_ENDPOINT,
//                                                     ZB_ZCL_CLUSTER_ID_METER_IDENTIFICATION,
//                                                     ZB_ZCL_CLUSTER_SERVER_ROLE,
//                                                     ZB_ZCl_ATTR_METER_IDENTIFICATION_POD);
//     if (erl_reporting_info)
//     {
//         ZB_ZCL_SET_REPORTING_FLAG(erl_reporting_info, ZB_ZCL_REPORT_ATTR);
//     }
//     else
//     {
//         LOG_INF("Could not obtain reporting info for ERL");
//     }

// 	ZB_SCHEDULE_APP_ALARM(update_attrs, 0, ERL_ATTRIBUTE_REPORT_INTERVAL);
// }


static void update_attrs(zb_uint8_t _)
{
	ZVUNUSED(_);
	zb_char_t erl_pod_buf[16] = "";
	zb_char_t erl_buf[16] = "";
	static zb_uint8_t erl_pod_idx;

	// Get a new Point Of Delivery
    switch ((erl_pod_idx++) % 5)
    {
        case 0:
            sprintf(erl_pod_buf, "twelve-chars");
            break;
        case 1:
            sprintf(erl_pod_buf, "eleven-char");
            break;
        case 2:
            sprintf(erl_pod_buf, "sevench");
            break;
        case 3:
            sprintf(erl_pod_buf, "eightchr");
            break;
		case 4:
			sprintf(erl_pod_buf, "sixchr");
			break;
    }

	ZB_ZCL_STRING_CLEAR(erl_buf);
    ZB_ZCL_STRING_APPEND_C_STR(erl_buf, 16, erl_pod_buf);

	zb_zcl_set_attr_val(
		ERL_ENDPOINT,
		ZB_ZCL_CLUSTER_ID_METER_IDENTIFICATION,
		ZB_ZCL_CLUSTER_SERVER_ROLE,
		ZB_ZCl_ATTR_METER_IDENTIFICATION_POD,
		erl_buf,
		ZB_FALSE
	);

	ZB_SCHEDULE_APP_ALARM(update_attrs, 0, ERL_ATTRIBUTE_REPORT_INTERVAL);
}

/**@brief Function to toggle the identify LED
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void toggle_identify_led(zb_bufid_t bufid)
{
	static int blink_status;

	dk_set_led(IDENTIFY_LED, (++blink_status) % 2);
	ZB_SCHEDULE_APP_ALARM(toggle_identify_led, bufid, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100));
}

/**@brief Function to handle identify notification events on the first endpoint.
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void identify_cb(zb_bufid_t bufid)
{
	zb_ret_t zb_err_code;

	if (bufid) {
		/* Schedule a self-scheduling function that will toggle the LED */
		ZB_SCHEDULE_APP_CALLBACK(toggle_identify_led, bufid);
	} else {
		/* Cancel the toggling function alarm and turn off LED */
		zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(toggle_identify_led, ZB_ALARM_ANY_PARAM);
		ZVUNUSED(zb_err_code);

		dk_set_led(IDENTIFY_LED, 0);
	}
}

/**@breif Starts identifying the device.
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void start_identifying(zb_bufid_t bufid)
{
	ZVUNUSED(bufid);
}

/**@brief Callback for button events.
 *
 * @param[in]   button_state  Bitmask containing buttons state.
 * @param[in]   has_changed   Bitmask containing buttons
 *                            that have changed their state.
 */
static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	/* Calculate bitmask of buttons that are pressed
	 * and have changed their state.
	 */
	uint32_t buttons = button_state & has_changed;

	if (buttons & IDENTIFY_MODE_BUTTON) {
		ZB_SCHEDULE_APP_CALLBACK(start_identifying, 0);
	}
}

/**@brief Function for initializing LEDs and Buttons. */
static void configure_gpio(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);
	}

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}
}

/**@brief Zigbee stack event handler.
 *
 * @param[in]   bufid   Reference to the Zigbee stack buffer
 *                      used to pass signal.
 */
void zboss_signal_handler(zb_bufid_t bufid)
{
	/* Update network status LED. */
	zigbee_led_status_update(bufid, ZIGBEE_NETWORK_STATE_LED);

	/* No application-specific behavior is required.
	 * Call default signal handler.
	 */
	ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));

	/* All callbacks should either reuse or free passed buffers.
	 * If bufid == 0, the buffer is invalid (not passed).
	 */
	if (bufid) {
		zb_buf_free(bufid);
	}
}

void main(void)
{
	LOG_INF("Starting Zigbee application ERL example");

	/* Initialize */
	configure_gpio();

	/* Register device context (endpoints). */
	ZB_AF_REGISTER_DEVICE_CTX(&app_template_ctx);

	app_clusters_attr_init();

	/* Register handlers to identify notifications */
	ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(ERL_ENDPOINT, identify_cb);

	ZB_SCHEDULE_APP_ALARM(update_attrs, 0, ERL_ATTRIBUTE_REPORT_INTERVAL);

	/* Start Zigbee default thread */
	zigbee_enable();

	LOG_INF("Zigbee application template started");
}
