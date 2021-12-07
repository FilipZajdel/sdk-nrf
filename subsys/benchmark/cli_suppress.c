/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include "cli_suppress.h"

#include "cli_api.h"
#include "nrf_log.h"

static bool m_suppress_cli = false;

void cli_suppress_enable(void)
{
    NRF_LOG_INFO("Suppressing CLI");

#if NRF_LOG_ENABLED
    while (NRF_LOG_PROCESS());
#endif

    cli_process();
    m_suppress_cli = true;
}

void cli_suppress_disable(void)
{
    m_suppress_cli = false;
    NRF_LOG_INFO("Enabling CLI");
}

bool cli_suppress_is_enabled(void)
{
    return m_suppress_cli;
}
