/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include "protocol_api.h"
#include "bsp.h"

#include "zboss_api.h"
#include "zb_ha_configuration_tool.h"
#include "zb_error_handler.h"
#include "zigbee_helpers.h"
#include "zigbee_cli.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define IEEE_CHANNEL_MASK                   (1l << ZIGBEE_CHANNEL)              /**< Scan only one, predefined channel to find the coordinator. */
#define ERASE_PERSISTENT_CONFIG             ZB_TRUE                             /**< Do not erase NVRAM to save the network parameters after device reboot or power-off. NOTE: If this option is set to ZB_TRUE then do full device erase for all network devices before running other samples. */
#define ZIGBEE_NETWORK_STATE_LED            BSP_BOARD_LED_2                     /**< LED indicating that light switch successfully joind Zigbee network. */

#if !defined ZB_ROUTER_ROLE
#error Define ZB_ROUTER_ROLE to compile CLI agent (Router) source code.
#endif

typedef struct cli_agent_ctx_s
{
  zb_uint8_t role;
} cli_agent_ctx_t;

static cli_agent_ctx_t    m_device_ctx;
static zb_uint8_t         m_attr_zcl_version   = ZB_ZCL_VERSION;
static zb_uint8_t         m_attr_power_source  = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;
static zb_uint16_t        m_attr_identify_time = 0;

/* Declare attribute list for Basic cluster. */
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &m_attr_zcl_version, &m_attr_power_source);

/* Declare attribute list for Identify cluster. */
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &m_attr_identify_time);

/* Declare cluster list for CLI Agent device. */
/* Only clusters Identify and Basic have attributes. */
ZB_HA_DECLARE_CONFIGURATION_TOOL_CLUSTER_LIST(cli_agent_clusters,
                                              basic_attr_list,
                                              identify_attr_list);

/* Declare endpoint for CLI Agent device. */
ZB_HA_DECLARE_CONFIGURATION_TOOL_EP(cli_agent_ep,
                                    ZIGBEE_CLI_ENDPOINT,
                                    cli_agent_clusters);

/* Declare application's device context (list of registered endpoints) for CLI Agent device. */
ZB_HA_DECLARE_CONFIGURATION_TOOL_CTX(cli_agent_ctx, cli_agent_ep);


/**@brief Zigbee stack event handler.
 *
 * @param[in]   bufid   Reference to the Zigbee stack buffer used to pass signal.
 */
void zboss_signal_handler(zb_bufid_t bufid)
{
    /* Update network status LED */
    zigbee_led_status_update(bufid, ZIGBEE_NETWORK_STATE_LED);

    /* Call default signal handler. */
    ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));

    if (bufid)
    {
        zb_buf_free(bufid);
    }
}

bool protocol_is_error(uint32_t error_code)
{
    return (zb_ret_t)error_code != RET_OK;
}

void protocol_init(void)
{
    zb_ieee_addr_t ieee_addr;

    UNUSED_VARIABLE(m_device_ctx);

    /* Intiialise the leds */
    bsp_board_leds_off();

    /* Set Zigbee stack logging level and traffic dump subsystem. */
    ZB_SET_TRACE_LEVEL(ZIGBEE_TRACE_LEVEL);
    ZB_SET_TRACE_MASK(ZIGBEE_TRACE_MASK);
    ZB_SET_TRAF_DUMP_OFF();

    /* Initialize Zigbee stack. */
    ZB_INIT("zigbee_benchmark");

    /* Set device address to the value read from FICR registers. */
    zb_osif_get_ieee_eui64(ieee_addr);
    zb_set_long_address(ieee_addr);

    zb_set_bdb_primary_channel_set(IEEE_CHANNEL_MASK);
    zb_set_nvram_erase_at_start(ERASE_PERSISTENT_CONFIG);

    /* Register CLI Agent device context (endpoints). */
    ZB_AF_REGISTER_DEVICE_CTX(&cli_agent_ctx);
}

void protocol_process(void)
{
    if (zb_cli_is_stack_started())
    {
        zboss_main_loop_iteration();
    }
}

void protocol_sleep(void)
{
}

void protocol_soc_evt_handler(uint32_t soc_evt)
{
    /* SOC events are passed to the ZBOSS stack via nRF sections */
    return;
}
