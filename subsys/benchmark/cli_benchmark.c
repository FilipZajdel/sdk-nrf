/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include <zephyr.h>
#include <assert.h>
#include <stdlib.h>
#include <shell/shell.h>
#include <logging/log.h>

#include <benchmark_cli_util.h>
#include "protocol_api.h"
// #include "cpu_utilization.h"
#include "cli_suppress.h"

LOG_MODULE_DECLARE(benchmark, CONFIG_LOG_DEFAULT_LEVEL);

#define DECIMAL_PRECISION 100
#define BPS_TO_KBPS       1024

static void print_test_results(benchmark_event_context_t * p_context);

static char long_log_buf[1024];
static int buf_pos;

struct shell const *p_shell;
static benchmark_peer_db_t *mp_peer_db;
static benchmark_configuration_t m_test_configuration =
{
    .length       = 64,
    .ack_timeout  = 200,
    .count        = 1000,
    .mode         = BENCHMARK_MODE_ACK,
};

static bool shell_help_requested(const struct shell *shell, int argc, char **argv)
{
    return strcmp(argv[0], "help") == 0;
}

static const char *configuration_mode_name_get(benchmark_mode_t * p_mode)
{
    switch (*(p_mode))
    {
        case BENCHMARK_MODE_UNIDIRECTIONAL:
        return "Unidirectional";

        case BENCHMARK_MODE_ECHO:
        return "Echo";

        case BENCHMARK_MODE_ACK:
        return "Ack";

        default:
        return "Unknown mode";
    }
}

static void discovered_peers_print(void *pp_shell, uint16_t event_size)
{
    struct shell *shell = *((struct shell **)pp_shell);

    (void)event_size;

    if (mp_peer_db != NULL)
    {
        protocol_cmd_peer_db_get(shell, mp_peer_db);
        print_done(shell);
    }
    else
    {
        print_error(shell, "The list of known peers is empty. Discover peers and try again.");
    }
}

static void benchmark_evt_handler(benchmark_evt_t * p_evt)
{
    uint16_t peer_count;

    switch (p_evt->evt)
    {
        case BENCHMARK_TEST_COMPLETED:
            LOG_INF("Test completed.");
            // cli_suppress_disable();
            print_test_results(&(p_evt->context));
            // benchmark_ble_flood_stop();
            break;

        case BENCHMARK_TEST_STARTED:
            assert(!protocol_is_error(p_evt->context.error));
            LOG_INF("Test started.");
            cli_suppress_enable();
            break;

        case BENCHMARK_TEST_STOPPED:
            cli_suppress_disable();

            if (!protocol_is_error(p_evt->context.error))
            {
                LOG_INF("Test successfully stopped.");
            }
            else
            {
                LOG_INF("Test stopped with errors. Error code: %u", p_evt->context.error);
                if (p_shell)
                {
                    shell_error(p_shell, "Error: Test stopped with errors. Error code: %u\r\n", p_evt->context.error);
                }
            }
            break;

        case BENCHMARK_DISCOVERY_COMPLETED:
            peer_count = p_evt->context.p_peer_information->peer_count;
            mp_peer_db = p_evt->context.p_peer_information;

            if (peer_count)
            {
                mp_peer_db->selected_peer  = peer_count - 1;
            }
            else
            {
                mp_peer_db = NULL;
            }

            LOG_INF("Discovery completed, found %d peers.", peer_count);

            discovered_peers_print(&p_shell, sizeof(p_shell));
            break;

        default:
            LOG_DBG("Unknown benchmark_evt.");
            break;
    };
}

const benchmark_peer_entry_t * benchmark_peer_selected_get(void)
{
    if (mp_peer_db == NULL)
    {
        return NULL;
    }

    return &(mp_peer_db->peer_table[mp_peer_db->selected_peer]);
}

/** Common commands, API used by all benchmark commands */
void print_done(const struct shell *shell)
{
    LOG_INF("Done");
}

void print_error(const struct shell *shell, char *what)
{
    LOG_ERR("Error: %s", what);
}

void cmd_default(const struct shell *shell, size_t argc, char ** argv)
{
    if ((argc == 1) || shell_help_requested(shell, argc, argv))
    {
        shell_help(shell);
    }
    else
    {
        shell_error(shell, "Error: %s: unknown parameter: %s\r\n", argv[0], argv[1]);
    }
}

/** Test configuration commands */
void cmd_config_get(const struct shell *shell, size_t argc, char ** argv)
{
    if (argc > 1)
    {
        // If unknown subcommand was passed.
        cmd_default(shell, argc, argv);
        return;
    }

    sprintf(long_log_buf, "=== Test settings ===\n"
            "Mode:%10s\nACK Timeout:%10d [ms]\nPacket count:%10d\nPayload length [B]:%10d",
            configuration_mode_name_get(&m_test_configuration.mode),
            m_test_configuration.ack_timeout,
            m_test_configuration.count,
            m_test_configuration.length);

    shell_info(shell, "%s", long_log_buf);
    // shell_info(shell, "=== Test settings ===");
    // shell_info(shell, "Mode:               %s", configuration_mode_name_get(&m_test_configuration.mode));
    // shell_info(shell, "ACK Timeout:        %d [ms]", m_test_configuration.ack_timeout);
    // shell_info(shell, "Packet count:       %d", m_test_configuration.count);
    // shell_info(shell, "Payload length [B]: %d", m_test_configuration.length);
}

static void cmd_info_get(const struct shell *shell, size_t argc, char ** argv)
{
    // Test settings
    cmd_config_get(shell, 0, NULL);

    // Local node information
    protocol_cmd_config_get(shell);

    // Peer information
    protocol_cmd_peer_get(shell, benchmark_peer_selected_get());
}

static void cmd_config_mode_get(const struct shell *shell, size_t argc, char **argv)
{
    if (argc > 1)
    {
        // If unknown subcommand was passed.
        cmd_default(shell, argc, argv);
        return;
    }

    shell_info(shell, "%s\r\n", configuration_mode_name_get(&m_test_configuration.mode));
    print_done(shell);
}

static void cmd_config_mode_unidirectional_set(const struct shell *shell, size_t argc, char ** argv)
{
    m_test_configuration.mode = BENCHMARK_MODE_UNIDIRECTIONAL;
    print_done(shell);
}

static void cmd_config_mode_echo_set(const struct shell *shell, size_t argc, char ** argv)
{
    m_test_configuration.mode = BENCHMARK_MODE_ECHO;
    print_done(shell);
}

static void cmd_config_mode_ack_set(const struct shell *shell, size_t argc, char ** argv)
{
    m_test_configuration.mode = BENCHMARK_MODE_ACK;
    print_done(shell);
}

static void cmd_config_ack_timeout(const struct shell *shell, size_t argc, char ** argv)
{
    if (argc > 2)
    {
        print_error(shell, "Too many arguments\r\n");
        return;
    }

    if (argc < 2)
    {
        shell_info(shell,"%d\r\n", m_test_configuration.ack_timeout);
    }
    else if (argc == 2)
    {
        m_test_configuration.ack_timeout = atoi(argv[1]);
    }

    print_done(shell);
}

static void cmd_config_packet_count(const struct shell *shell, size_t argc, char ** argv)
{
    if (argc > 2)
    {
        print_error(shell, "Too many arguments\r\n");
        return;
    }

    if (argc < 2)
    {
        shell_info(shell,"%d\r\n", m_test_configuration.count);
    }
    else if (argc == 2)
    {
        m_test_configuration.count = atoi(argv[1]);
    }

    print_done(shell);
}

static void cmd_config_packet_length(const struct shell *shell, size_t argc, char ** argv)
{
    if (argc > 2)
    {
        print_error(shell, "Too many arguments.");
        return;
    }

    if (argc < 2)
    {
        shell_info(shell,"%d", m_test_configuration.length);
    }
    else if (argc == 2)
    {
        m_test_configuration.length = atoi(argv[1]);
    }

    print_done(shell);
}

/** Peer configuration commands */
static void cmd_discover_peers(const struct shell *shell, size_t argc, char ** argv)
{
    uint32_t err_code;

    // Remember cli used to start the test so results can be printed on the same interface.
    p_shell = shell;

    err_code = benchmark_test_init(&m_test_configuration, benchmark_evt_handler);
    if (protocol_is_error(err_code))
    {
        print_error(shell, "Failed to configure discovery parameters.");
    }

    err_code = benchmark_peer_discover();
    if (protocol_is_error(err_code))
    {
        shell_error(shell, "Failed to sent discovery message.");
    }
}

static void cmd_display_peers(const struct shell *shell, size_t argc, char **argv)
{
    discovered_peers_print(&shell, 0);
}

static void cmd_peer_select(const struct shell *shell, size_t argc, char **argv)
{
    if (mp_peer_db == NULL)
    {
        print_error(shell, "The list of known peers is empty. Discover peers and try again.");
        return;
    }

    if (argc > 2)
    {
        print_error(shell, "Too many arguments.");
        return;
    }

    if (argc == 1)
    {
        shell_info(shell, "%d\r\n", mp_peer_db->selected_peer);
        print_done(shell);
        return;
    }

    if (mp_peer_db->peer_count > atoi(argv[1]))
    {
        mp_peer_db->selected_peer = atoi(argv[1]);
        print_done(shell);
    }
    else
    {
        print_error(shell, "Peer index out of range.\n");
    }
}

static void parse_decimal(char *buf_out, const char *p_description, const char *p_unit, uint32_t value)
{
    if (value != BENCHMARK_COUNTERS_VALUE_NOT_SUPPORTED)
    {
        uint32_t value_int       = value / DECIMAL_PRECISION;
        uint32_t value_remainder = value % DECIMAL_PRECISION;

        sprintf(buf_out, "%s: %lu.%02lu%s", p_description, value_int, value_remainder, p_unit);
    }
    else
    {
        LOG_INF("%s: Not supported\n", p_description);
    }
}

static void dump_config(benchmark_configuration_t * p_config)
{
    const char * const modes[] = {"Unidirectional", "Echo", "ACK"};

    buf_pos += sprintf(long_log_buf + buf_pos,
                       "\r\n        Length: %u"
                       "\r\n        ACK timeout: %u ms"
                       "\r\n        Count: %u"
                       "\r\n        Mode: %s",
                       p_config->length, p_config->ack_timeout, p_config->count,
                       modes[p_config->mode]);
}

static void dump_status(benchmark_status_t * p_status)
{
    buf_pos += sprintf(long_log_buf + buf_pos,
                      "\r\n        Test in progress: %s"
                      "\r\n        Reset counters: %s"
                      "\r\n        ACKs lost: %u"
                      "\r\n        Waiting for ACKs: %u"
                      "\r\n        Packets left count: %u"
                      "\r\n        Frame number: %u",
                      p_status->test_in_progress ? "True" : "False",
                      p_status->reset_counters ? "True" : "False",
                      p_status->acks_lost,
                      p_status->waiting_for_ack,
                      p_status->packets_left_count,
                      p_status->frame_number);

    if (m_test_configuration.mode == BENCHMARK_MODE_ECHO)
    {
        uint32_t avg = 0;
        char latency_min[25] = "";
        char latency_max[25] = "";
        char latency_avg[25] = "";

        if (p_status->latency.cnt > 0)
        {
            avg = (uint32_t)(p_status->latency.sum * DECIMAL_PRECISION / (p_status->latency.cnt));
        }

        parse_decimal(latency_min, "Min", "ms", p_status->latency.min * DECIMAL_PRECISION);
        parse_decimal(latency_max, "Max", "ms", p_status->latency.max * DECIMAL_PRECISION);
        parse_decimal(latency_avg, "Avg", "ms", avg);

        buf_pos += sprintf(long_log_buf + buf_pos, "\r\n        Latency:"
                                                   "\r\n           %s"
                                                   "\r\n           %s"
                                                   "\r\n           %s",
                           latency_min, latency_max, latency_avg);
    }

    LOG_INF("\r\n%s", log_strdup(long_log_buf));
    buf_pos = 0;
}

static void dump_result(benchmark_result_t * p_result)
{
    char cpu_util_buf[40] = "";

    parse_decimal(cpu_util_buf, "        CPU utilization", "%", p_result->cpu_utilization * DECIMAL_PRECISION / 100);

    buf_pos += sprintf(long_log_buf + buf_pos, "%s"
                          "\r\n        Duration: %u ms"
                          "\r\n        App counters:"
                          "\r\n            Bytes received: %uB"
                          "\r\n            Packets received: %u"
                          "\r\n            RX error: %u"
                          "\r\n            RX total: %u"
                          "\r\n        Mac counters:"
                          "\r\n            TX error: %u"
                          "\r\n            TX total: %u",
                          cpu_util_buf, p_result->duration, p_result->rx_counters.bytes_received,
                          p_result->rx_counters.packets_received, p_result->rx_counters.rx_error,
                          p_result->rx_counters.rx_total, p_result->mac_tx_counters.error,
                          p_result->mac_tx_counters.total);

    LOG_INF("\r\n%s", log_strdup(long_log_buf));
    buf_pos = 0;
    // print_decimal(p_shell, "        CPU utilization", "%", p_result->cpu_utilization * DECIMAL_PRECISION / 100);
}

// static void dump_ble_result(benchmark_ble_results_t * p_result)
// {
//     print_int(p_shell,     "        Bytes transferred", "B", p_result->bytes_transfered);
//     print_int(p_shell,     "        Duration", "ms", p_result->duration);
//     print_decimal(p_shell, "        Throughput", "kbps", p_result->throughput * DECIMAL_PRECISION / BPS_TO_KBPS);
// }

/** Test execution commands */
static void print_test_results(benchmark_event_context_t * p_context)
{
    benchmark_evt_results_t *p_results = &p_context->results;

    if (p_results->p_remote_result == NULL)
    {
        return;
    }

    buf_pos += sprintf(long_log_buf, "\r\n=== Test Finished ===");

    if ((p_results->p_local_status != NULL) && (p_results->p_local_result != NULL) && (p_results->p_local_result->duration != 0))
    {
        uint32_t test_duration                            = p_results->p_local_result->duration;
        uint32_t packets_sent                             = m_test_configuration.count - p_results->p_local_status->packets_left_count;
        uint32_t packets_acked                            = packets_sent - p_results->p_local_status->acks_lost;
        uint32_t txed_bytes                               = m_test_configuration.length * packets_sent;
        uint32_t acked_bytes                              = m_test_configuration.length * packets_acked;
        uint32_t txed_bits                                = m_test_configuration.length * packets_sent * 8;
        uint32_t throughput                               = (uint32_t)(txed_bits / test_duration);
        uint32_t throughput_rtx                           = (uint32_t)((acked_bytes * 1000ULL) / (test_duration * 128ULL));
        uint32_t latency_sum                              = (uint32_t)p_results->p_local_status->latency.sum*2;

        char throughput_str[60] = "";
        char throughput_rtx_str[60] = "";

        // parse_decimal(throughput_str, "    Throughput", "kbps", txed_bits * DECIMAL_PRECISION / test_duration);
        // parse_decimal(throughput_rtx_str, "    Throughput", "kbps", acked_bytes * DECIMAL_PRECISION * 1000ULL / (test_duration * 128ULL));


        parse_decimal(throughput_str, "    Throughput", "kbps", txed_bits * DECIMAL_PRECISION / latency_sum);
        parse_decimal(throughput_rtx_str, "    Throughput", "kbps", acked_bytes * DECIMAL_PRECISION * 1000ULL / (latency_sum * 128ULL));


        buf_pos += sprintf(long_log_buf + buf_pos,
                           "\r\nsent packets: %u\r\nacked packets: %u\r\ntxed bytes: %u\r\nacked bytes: %u",
                           packets_sent, packets_acked, txed_bytes, acked_bytes);

        buf_pos += sprintf(long_log_buf + buf_pos, "\r\nTest duration: %u ms", test_duration);

        if (m_test_configuration.mode == BENCHMARK_MODE_ECHO)
        {
            char avg_str[40] = "";
            uint32_t avg = 0;

            if (p_results->p_local_status->latency.cnt > 0)
            {
                avg = (uint32_t)(p_results->p_local_status->latency.sum * DECIMAL_PRECISION / p_results->p_local_status->latency.cnt);
            }

            parse_decimal(avg_str, "    Avg", "", avg);

            buf_pos += sprintf(long_log_buf + buf_pos,
                               "\r\nLatency:"
                               "\r\n    Min: %u"
                               "\r\n    Max: %u"
                               "\r\n%s",
                               p_results->p_local_status->latency.min,
                               p_results->p_local_status->latency.max,
                               avg_str);

        }

        buf_pos += sprintf(long_log_buf + buf_pos, "\r\nAverage CPU utilization:");

        // LOG_INF("Average CPU utilization:");
        // print_decimal(p_shell, "    Local", "%", p_results->p_local_result->cpu_utilization * DECIMAL_PRECISION / 100);

        // if (p_results->p_remote_result != NULL)
        // {
        //     print_decimal(p_shell, "    Remote", "%", p_results->p_remote_result->cpu_utilization * DECIMAL_PRECISION / 100);
        // }

        if (m_test_configuration.mode == BENCHMARK_MODE_UNIDIRECTIONAL)
        {
            buf_pos += sprintf(long_log_buf + buf_pos,
                               "\r\nUnidirectional:"
                               "\r\n%s", throughput_str);
        }
        else
        {
            uint32_t per = UINT32_MAX;
            char per_str[2][40] = {"", ""};

            if (packets_sent != 0)
            {
                per = (uint32_t)((DECIMAL_PRECISION * 100ULL * (packets_sent - packets_acked)) / packets_sent);
            }

            parse_decimal(per_str[0], "    PER", "%", per);
            parse_decimal(per_str[1], "    PER", "%", 0);

            buf_pos += sprintf(long_log_buf + buf_pos, "\r\nWithout retransmissions:"
                               "\r\n%s" "\r\n%s"
                               "\r\nWith retransmissions:"
                               "\r\n%s" "\r\n%s",
                               per_str[0], throughput_str, per_str[1], throughput_rtx_str);
        }

        if (m_test_configuration.mode == BENCHMARK_MODE_UNIDIRECTIONAL)
        {
            uint32_t mac_tx_attempts = p_results->p_local_result->mac_tx_counters.total;
            uint32_t mac_tx_errors   = p_results->p_local_result->mac_tx_counters.error;
            uint32_t mac_per         = UINT32_MAX;
            char mac_per_str[40]     = "";

            if (mac_tx_attempts != 0)
            {
                mac_per = (uint32_t)((DECIMAL_PRECISION * 100ULL * mac_tx_errors) / mac_tx_attempts);
            }

            parse_decimal(mac_per_str, "MAC PER", "%", mac_per);
            buf_pos += sprintf(long_log_buf + buf_pos, "\r\n%s", mac_per_str);
        }
        else
        {
            if (p_results->p_remote_result != NULL)
            {
                uint32_t mac_tx_attempts = p_results->p_local_result->mac_tx_counters.total + p_results->p_remote_result->mac_tx_counters.total;
                uint32_t mac_tx_errors   = p_results->p_local_result->mac_tx_counters.error + p_results->p_remote_result->mac_tx_counters.error;
                uint32_t mac_per         = UINT32_MAX;
                char mac_per_str[40]     = "";

                if (mac_tx_attempts != 0)
                {
                    mac_per = (uint32_t)((DECIMAL_PRECISION * 100ULL * mac_tx_errors) / mac_tx_attempts);
                }

                parse_decimal(mac_per_str, "MAC PER", "%", mac_per);
                // print_decimal(p_shell, "MAC PER", "%", mac_per);
                buf_pos += sprintf(long_log_buf + buf_pos, "\r\n%s", mac_per_str);
            }
            else
            {

                buf_pos += sprintf(long_log_buf + buf_pos,
                                   "\r\nMAC Counters:"
                                   "\r\n    MAC TX Total: %u"
                                   "\r\n    MAC TX Err: %u",
                                   p_results->p_local_result->mac_tx_counters.total,
                                   p_results->p_local_result->mac_tx_counters.error);
                // LOG_INF("MAC Counters:");
                // print_int(p_shell, "    MAC TX Total", "", p_results->p_local_result->mac_tx_counters.total);
                // print_int(p_shell, "    MAC TX Err", "",   p_results->p_local_result->mac_tx_counters.error);
            }
        }

        LOG_INF("\r\n%s", log_strdup(long_log_buf));
        buf_pos = 0;

        buf_pos += sprintf(long_log_buf+buf_pos,
                           "\r\nRaw data:"
                           "\r\n    Config:");
        dump_config(&m_test_configuration);

        buf_pos += sprintf(long_log_buf + buf_pos,
                           "\r\n    Status:");
        dump_status(p_results->p_local_status);

        buf_pos += sprintf(long_log_buf + buf_pos, "\r\n    Local:");
        dump_result(p_results->p_local_result);

        if (p_results->p_remote_result != NULL)
        {
            buf_pos += sprintf(long_log_buf + buf_pos, "\r\n    Remote:");
            dump_result(p_results->p_remote_result);
        }

        // if (p_ble_ping_results != NULL)
        // {
        //     shell_info(p_shell, "    BLE local:\r\n");
        //     dump_ble_result(&p_ble_ping_results->local_results);

        //     shell_info(p_shell, "    BLE remote:\r\n");
        //     dump_ble_result(&p_ble_ping_results->remote_results);
        // }
    }

    print_done(p_shell);
}

static void cmd_test_start(const struct shell *shell, size_t argc, char ** argv)
{
    uint32_t err_code;

    if (mp_peer_db == NULL)
    {
        shell_error(shell, "No peer selected; run:\n     test peer discover \nto find peers\n");
        return;
    }

    // Remember cli used to start the test so results can be printed on the same interface.
    p_shell = shell;

    // benchmark_ble_flood_start();

    err_code = benchmark_test_init(&m_test_configuration, benchmark_evt_handler);
    if (protocol_is_error(err_code))
    {
        print_error(shell, "Failed to configure test parameters.");
        return;
    }

    err_code = benchmark_test_start();
    if (protocol_is_error(err_code))
    {
        print_error(shell, "Failed to start test.");
    }
}

static void cmd_test_stop(const struct shell *shell, size_t argc, char ** argv)
{
    uint32_t err_code = benchmark_test_stop();
    if (protocol_is_error(err_code))
    {
        print_error(shell, "Failed to stop test.");
    }
    else
    {
        print_done(shell);
    }
}

static void cmd_peer_test_results(const struct shell *shell, size_t argc, char ** argv)
{
    uint32_t err_code = benchmark_peer_results_request_send();

    if (protocol_is_error(err_code))
    {
        print_error(shell, "Failed to send test results request.");
        return;
    }
}

SHELL_STATIC_SUBCMD_SET_CREATE(test_configure_mode,
    SHELL_CMD_ARG(ack, NULL, "Peer replies with a short acknowledgment",
                  cmd_config_mode_ack_set, 1, 0),
    SHELL_CMD_ARG(echo, NULL, "Peer replies with the same data",
                  cmd_config_mode_echo_set, 1, 0),
    SHELL_CMD_ARG(unidirectional, NULL, "Transmission in the single direction",
                  cmd_config_mode_unidirectional_set, 1, 0),
    SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(test_configure_peer,
    SHELL_CMD_ARG(discover, NULL, "Discover available peers", cmd_discover_peers, 1, 0),
    SHELL_CMD_ARG(list,     NULL, "Display found peers",      cmd_display_peers, 1, 0),
    SHELL_CMD_ARG(results,  NULL, "Display results",          cmd_peer_test_results, 1, 0),
    SHELL_CMD_ARG(select,   NULL, "Select peer from a list",  cmd_peer_select, 1, 1),
    SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(test_configure_cmds,
    SHELL_CMD_ARG(ack-timeout, NULL,
        "[Not supported] Set time after we stop waiting for the acknowledgment from the peer in milliseconds",
                  cmd_config_ack_timeout, 1, 1),
    SHELL_CMD_ARG(count, NULL, "Set number of packets to be sent",
                  cmd_config_packet_count, 1, 1),
    SHELL_CMD_ARG(length, NULL, "Set UDP payload length in bytes",
                  cmd_config_packet_length, 1, 1),
    SHELL_CMD_ARG(mode, &test_configure_mode, "Set test type",
                  cmd_config_mode_get, 2, 0),
    SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(benchmark_cmds,
    SHELL_CMD_ARG(configure, &test_configure_cmds, "Configure the test",
                  cmd_default, 1, 1),
    SHELL_CMD_ARG(info, NULL, "Print current configuration", cmd_info_get, 1, 0),
    SHELL_CMD_ARG(peer, &test_configure_peer, "Configure the peer receiving data",
                  cmd_default, 1, 0),
    SHELL_CMD_ARG(start, NULL, "Start the test", cmd_test_start, 1, 0),
    SHELL_CMD_ARG(stop, NULL, "Stop the test", cmd_test_stop, 1, 0),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(test, &benchmark_cmds, "Benchmark commands", cmd_default);
