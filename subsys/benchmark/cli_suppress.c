/*$$$LICENCE_NORDIC_STANDARD<2022>$$$*/
#include <logging/log.h>
#include <shell/shell.h>

LOG_MODULE_DECLARE(benchmark, CONFIG_LOG_DEFAULT_LEVEL);

static bool m_suppress_cli = false;
static struct shell *shell = NULL;

void cli_suppress_init(struct shell *_shell)
{
    shell = _shell;
}

void cli_suppress_enable(void)
{
    shell_info(shell, "Suppressing CLI");

    m_suppress_cli = true;

    shell_stop(shell);
}

void cli_suppress_disable(void)
{
    shell_info(shell, "Enabling CLI");

    m_suppress_cli = false;

    shell_start(shell);
}

bool cli_suppress_is_enabled(void)
{
    return m_suppress_cli;
}
