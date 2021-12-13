/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include "protocol_api.h"
#include "benchmark_zigbee_common.h"
#include "commands.h"

void protocol_cmd_peer_db_get(const nrf_cli_t * p_cli, const benchmark_peer_db_t * p_peers)
{
    nrf_cli_fprintf(p_cli,
                    NRF_CLI_INFO,
                    "\r\n# ||    Device ID   || 16-bit network address\r\n");

    for (uint16_t i = 0; i < p_peers->peer_count; i++)
    {
        nrf_cli_fprintf(p_cli,
                        NRF_CLI_INFO,
                        "%d:  %08x%08x  %04x\r\n",
                        i,
                        DEVICE_ID_HI(p_peers->peer_table[i].device_id),
                        DEVICE_ID_LO(p_peers->peer_table[i].device_id),
                        p_peers->peer_table[i].p_address->nwk_addr);
    }
}

void protocol_cmd_config_get(const nrf_cli_t * p_cli)
{
    zb_nwk_device_type_t role;
    zb_ieee_addr_t       addr;
    zb_uint16_t          short_addr;
    uint64_t             device_id = benchmark_local_device_id_get();

    zb_get_long_address(addr);
    short_addr = zb_address_short_by_ieee(addr);

    nrf_cli_fprintf(p_cli, NRF_CLI_INFO, "\r\n\t=== Local node information ===\r\n");
    nrf_cli_fprintf(p_cli, NRF_CLI_INFO,
                    "Device ID:   %08x%08x\r\n",
                    DEVICE_ID_HI(device_id),
                    DEVICE_ID_LO(device_id));

    if (short_addr != ZB_UNKNOWN_SHORT_ADDR)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_INFO, "Network address: %04x\r\n", short_addr);
    }
    else
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_INFO, "Network address: none\r\n");
    }

    nrf_cli_fprintf(p_cli, NRF_CLI_INFO, "Zigbee Role: ");
    role = zb_get_network_role();
    if (role == ZB_NWK_DEVICE_TYPE_NONE)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_INFO, "unknown");
    }
    else if (role == ZB_NWK_DEVICE_TYPE_COORDINATOR)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_INFO, "zc");
    }
    else if (role == ZB_NWK_DEVICE_TYPE_ROUTER)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_INFO, "zr");
    }
    else if (role == ZB_NWK_DEVICE_TYPE_ED)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_INFO, "zed");
    }
    nrf_cli_fprintf(p_cli, NRF_CLI_INFO, "\r\n");
}

void protocol_cmd_peer_get(const nrf_cli_t * p_cli, const benchmark_peer_entry_t * p_peer)
{
    uint32_t device_id_lo = 0;
    uint32_t device_id_hi = 0;
    uint16_t short_addr   = 0xFFFF;

    if (p_peer)
    {
        device_id_lo = DEVICE_ID_LO(p_peer->device_id);
        device_id_hi = DEVICE_ID_HI(p_peer->device_id);
        short_addr   = p_peer->p_address->nwk_addr;
    }

    nrf_cli_fprintf(p_cli, NRF_CLI_INFO, "\r\n\t=== Peer information ===\r\n");
    nrf_cli_fprintf(p_cli, NRF_CLI_INFO, "Device ID:   %08x%08x\r\n", device_id_hi, device_id_lo);
    nrf_cli_fprintf(p_cli, NRF_CLI_INFO, "Network address: %04x\r\n", short_addr);
}

void protocol_cmd_remote_send(const nrf_cli_t * p_cli, const nrf_cli_t * p_peer_cli, size_t argc, char ** argv)
{
    /* Remote command execution is not implemented in Zigbee benchmark. */
    return;
}
