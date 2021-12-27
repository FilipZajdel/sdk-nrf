/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include <zephyr.h>
#include <assert.h>
#include <stdlib.h>
#include <shell/shell.h>

#include <benchmark_cli_util.h>
#include "protocol_api.h"
// #include "cpu_utilization.h"
#include "cli_suppress.h"

#define DECIMAL_PRECISION 100
#define BPS_TO_KBPS       1024

static void print_test_results(benchmark_event_context_t * p_context);

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
            printk("Test completed.");
            cli_suppress_disable();
            print_test_results(&(p_evt->context));
            // benchmark_ble_flood_stop();
            break;

        case BENCHMARK_TEST_STARTED:
            assert(!protocol_is_error(p_evt->context.error));
            printk("Test started.");
            cli_suppress_enable();
            break;

        case BENCHMARK_TEST_STOPPED:
            cli_suppress_disable();

            if (!protocol_is_error(p_evt->context.error))
            {
                printk("Test successfully stopped.");
            }
            else
            {
                printk("Test stopped with errors. Error code: %u", p_evt->context.error);
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

            printk("Discovery completed, found %d peers.", peer_count);

            // err_code = app_sched_event_put(&p_shell, sizeof(p_shell), discovered_peers_print);
            // ASSERT(err_code == NRF_SUCCESS);
            discovered_peers_print(&p_shell, sizeof(p_shell));
            break;

        default:
            printk("Unknown benchmark_evt.");
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
    shell_info(shell, "Done\r\n");
}

void print_error(const struct shell *shell, char *what)
{
    shell_error(shell, "Error: %s\r\n", what);
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

    shell_info(shell, "\n    === Test settings ===\r\n");
    shell_info(shell, "Mode:               %s\r\n", configuration_mode_name_get(&m_test_configuration.mode));
    shell_info(shell, "ACK Timeout:        %d [ms]\r\n", m_test_configuration.ack_timeout);
    shell_info(shell, "Packet count:       %d\r\n", m_test_configuration.count);
    shell_info(shell, "Payload length [B]: %d\r\n", m_test_configuration.length);
}

static void cmd_info_get(const struct shell *shell, size_t argc, char ** argv)
{
    // Test settings
    cmd_config_get(shell, 0, NULL);

    // Local node information
    protocol_cmd_config_get(shell);

    // Peer information
    protocol_cmd_peer_get(shell, benchmark_peer_selected_get());

    print_done(shell);
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
        shell_info(shell,"%d\r\n", m_test_configuration.length);
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
        print_error(shell, "Peer index out of range.");
    }
}

static void print_int(const struct shell *shell, const char *p_description, const char *p_unit, uint32_t value)
{
    if (value != BENCHMARK_COUNTERS_VALUE_NOT_SUPPORTED)
    {
        shell_info(shell, "%s: %lu%s\r\n", p_description, value, p_unit);
    }
    else
    {
        shell_info(shell, "%s: Not supported\r\n", p_description);
    }
}

static void print_decimal(const struct shell *shell, const char *p_description, const char *p_unit, uint32_t value)
{
    if (value != BENCHMARK_COUNTERS_VALUE_NOT_SUPPORTED)
    {
        uint32_t value_int       = value / DECIMAL_PRECISION;
        uint32_t value_remainder = value % DECIMAL_PRECISION;

        shell_info(shell, "%s: %lu.%02lu%s\r\n", p_description, value_int, value_remainder, p_unit);
    }
    else
    {
        shell_info(shell, "%s: Not supported\r\n", p_description);
    }
}

static void dump_config(benchmark_configuration_t * p_config)
{
    const char * const modes[] = {"Unidirectional", "Echo", "ACK"};

    print_int(p_shell, "        Length", "", p_config->length);
    print_int(p_shell, "        ACK timeout", "ms", p_config->ack_timeout);
    print_int(p_shell, "        Count", "", p_config->count);
    shell_info(p_shell, "        Mode: %s\r\n", modes[p_config->mode]);
}

static void dump_status(benchmark_status_t * p_status)
{
    shell_info(p_shell, "        Test in progress: %s\r\n", p_status->test_in_progress ? "True" : "False");
    shell_info(p_shell, "        Reset counters: %s\r\n", p_status->reset_counters ? "True" : "False");
    print_int(p_shell, "        ACKs lost", "", p_status->acks_lost);
    print_int(p_shell, "        Waiting for ACKs", "", p_status->waiting_for_ack);
    print_int(p_shell, "        Packets left count", "", p_status->packets_left_count);
    print_int(p_shell, "        Frame number", "", p_status->frame_number);


    if (m_test_configuration.mode == BENCHMARK_MODE_ECHO)
    {
        uint32_t avg = 0;
        if (p_status->latency.cnt > 0)
        {
            avg = (uint32_t)(p_status->latency.sum / p_status->latency.cnt);
        }

        shell_info(p_shell, "        Latency:\r\n");
        print_decimal(p_shell, "            Min", "ms", p_status->latency.min * DECIMAL_PRECISION / 1000);
        print_decimal(p_shell, "            Max", "ms", p_status->latency.max * DECIMAL_PRECISION / 1000);
        print_decimal(p_shell, "            Avg", "ms", avg * DECIMAL_PRECISION / 1000);
    }
}

static void dump_result(benchmark_result_t * p_result)
{
    print_decimal(p_shell, "        CPU utilization", "%", p_result->cpu_utilization * DECIMAL_PRECISION / 100);
    print_int(p_shell, "        Duration", "ms", p_result->duration);

    shell_info(p_shell, "        App counters:\r\n");

    print_int(p_shell, "            Bytes received", "B", p_result->rx_counters.bytes_received);
    print_int(p_shell, "            Packets received", "", p_result->rx_counters.packets_received);
    print_int(p_shell, "            RX error", "", p_result->rx_counters.rx_error);
    print_int(p_shell, "            RX total", "", p_result->rx_counters.rx_total);

    shell_info(p_shell, "        Mac counters:\r\n");
    print_int(p_shell, "            TX error", "", p_result->mac_tx_counters.error);
    print_int(p_shell, "            TX total", "", p_result->mac_tx_counters.total);
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
    benchmark_evt_results_t * p_results = &p_context->results;

    if (p_results->p_remote_result == NULL)
    {
        return;
    }

    shell_info(p_shell, "\r\n    === Test Finished ===\r\n");

    if ((p_results->p_local_status != NULL) && (p_results->p_local_result != NULL) && (p_results->p_local_result->duration != 0))
    {
        uint32_t test_duration                            = p_results->p_local_result->duration;
        uint32_t packets_sent                             = m_test_configuration.count - p_results->p_local_status->packets_left_count;
        uint32_t packets_acked                            = packets_sent - p_results->p_local_status->acks_lost;
        uint32_t txed_bytes                               = m_test_configuration.length * packets_sent;
        uint32_t acked_bytes                              = m_test_configuration.length * packets_acked;
        uint32_t throughput                               = (uint32_t)((txed_bytes * 1000ULL) / (test_duration * 128ULL));
        uint32_t throughput_rtx                           = (uint32_t)((acked_bytes * 1000ULL) / (test_duration * 128ULL));

        // benchmark_ble_ping_results_t * p_ble_ping_results = benchmark_ble_continuous_results_get();

        print_int(p_shell, "Test duration", "ms", p_results->p_local_result->duration);

        shell_info(p_shell, "\r\n");

        if (m_test_configuration.mode == BENCHMARK_MODE_ECHO)
        {
            uint32_t avg = 0;
            if (p_results->p_local_status->latency.cnt > 0)
            {
                avg = (uint32_t)(p_results->p_local_status->latency.sum / p_results->p_local_status->latency.cnt);
            }

            shell_info(p_shell, "Latency:\r\n");
            print_decimal(p_shell, "    Min", "ms", p_results->p_local_status->latency.min * DECIMAL_PRECISION / 1000);
            print_decimal(p_shell, "    Max", "ms", p_results->p_local_status->latency.max * DECIMAL_PRECISION / 1000);
            print_decimal(p_shell, "    Avg", "ms", avg * DECIMAL_PRECISION / 1000);

            shell_info(p_shell, "\r\n");
        }

        shell_info(p_shell, "Average CPU utilization:\r\n");
        print_decimal(p_shell, "    Local", "%", p_results->p_local_result->cpu_utilization * DECIMAL_PRECISION / 100);

        if (p_results->p_remote_result != NULL)
        {
            print_decimal(p_shell, "    Remote", "%", p_results->p_remote_result->cpu_utilization * DECIMAL_PRECISION / 100);
        }

        shell_info(p_shell, "\r\n");

        if (m_test_configuration.mode == BENCHMARK_MODE_UNIDIRECTIONAL)
        {
            shell_info(p_shell, "Unidirectional:\r\n");
            shell_info(p_shell,"    Throughput: %lu kbps\r\n", throughput);
        }
        else
        {
            uint32_t per = UINT32_MAX;

            if (packets_sent != 0)
            {
                per = (uint32_t)((DECIMAL_PRECISION * 100ULL * (packets_sent - packets_acked)) / packets_sent);
            }

            shell_info(p_shell, "Without retransmissions:\r\n");
            print_decimal(p_shell, "    PER", "%", per);
            shell_info(p_shell, "    Throughput: %lu kbps\r\n", throughput);

            shell_info(p_shell, "\r\n");

            shell_info(p_shell, "With retransmissions:\r\n");
            print_decimal(p_shell, "    PER", "%", 0);
            shell_info(p_shell, "    Throughput: %lu kbps\r\n", throughput_rtx);
        }

        shell_info(p_shell, "\r\n");

        if (m_test_configuration.mode == BENCHMARK_MODE_UNIDIRECTIONAL)
        {
            uint32_t mac_tx_attempts = p_results->p_local_result->mac_tx_counters.total;
            uint32_t mac_tx_errors   = p_results->p_local_result->mac_tx_counters.error;
            uint32_t mac_per         = UINT32_MAX;
            if (mac_tx_attempts != 0)
            {
                mac_per = (uint32_t)((DECIMAL_PRECISION * 100ULL * mac_tx_errors) / mac_tx_attempts);
            }
            print_decimal(p_shell, "MAC PER", "%", mac_per);
        }
        else
        {
            if (p_results->p_remote_result != NULL)
            {
                uint32_t mac_tx_attempts = p_results->p_local_result->mac_tx_counters.total + p_results->p_remote_result->mac_tx_counters.total;
                uint32_t mac_tx_errors   = p_results->p_local_result->mac_tx_counters.error + p_results->p_remote_result->mac_tx_counters.error;
                uint32_t mac_per         = UINT32_MAX;
                if (mac_tx_attempts != 0)
                {
                    mac_per = (uint32_t)((DECIMAL_PRECISION * 100ULL * mac_tx_errors) / mac_tx_attempts);
                }
                print_decimal(p_shell, "MAC PER", "%", mac_per);
            }
            else
            {
                shell_info(p_shell, "MAC Counters:\r\n");
                print_int(p_shell, "    MAC TX Total", "", p_results->p_local_result->mac_tx_counters.total);
                print_int(p_shell, "    MAC TX Err", "", p_results->p_local_result->mac_tx_counters.error);
            }
        }

        shell_info(p_shell, "\r\n");
        shell_info(p_shell, "Raw data:\r\n");
        shell_info(p_shell, "    Config:\r\n");
        dump_config(&m_test_configuration);

        shell_info(p_shell, "    Status:\r\n");
        dump_status(p_results->p_local_status);

        shell_info(p_shell, "    Local:\r\n");
        dump_result(p_results->p_local_result);

        if (p_results->p_remote_result != NULL)
        {
            shell_info(p_shell, "    Remote:\r\n");
            dump_result(p_results->p_remote_result);
        }

        // if (p_ble_ping_results != NULL)
        // {
        //     shell_info(p_shell, "    BLE local:\r\n");
        //     dump_ble_result(&p_ble_ping_results->local_results);

        //     shell_info(p_shell, "    BLE remote:\r\n");
        //     dump_ble_result(&p_ble_ping_results->remote_results);
        // }

        shell_info(p_shell, "\r\n");
    }

    print_done(p_shell);
}

static void cmd_test_start(const struct shell *shell, size_t argc, char ** argv)
{
    uint32_t err_code;

    if (mp_peer_db == NULL)
    {
        shell_error(shell, "No peer selected; run:\r\n     test peer discover \r\nto find peers\r\n");
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
        "Set time after we stop waiting for the acknowledgment from the peer in milliseconds",
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
