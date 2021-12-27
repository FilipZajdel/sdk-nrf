/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include <zephyr.h>
#include "cli_suppress.h"

#include "cli_api.h"

static bool m_suppress_cli = false;

void cli_suppress_enable(void)
{
    printk("Suppressing CLI");

    m_suppress_cli = true;
}

void cli_suppress_disable(void)
{
    m_suppress_cli = false;
    printk("Enabling CLI");
}

bool cli_suppress_is_enabled(void)
{
    return m_suppress_cli;
}
