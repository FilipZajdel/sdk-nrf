/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include "protocol_api.h"

#include <logging/log.h>
#include <zboss_api.h>
#include <zigbee_cli.h>

#include "zb_ha_configuration_tool.h"
#include <zigbee/zigbee_error_handler.h>

#define ZIGBEE_CLI_ENDPOINT                 64

bool protocol_is_error(uint32_t error_code)
{
    return (zb_ret_t)error_code != RET_OK;
}
