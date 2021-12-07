#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include)

message("Including benchmark files")

zephyr_library()

zephyr_library_sources(
  benchmark_util.c
  cli_cpu.c
)

# zephyr_library_sources_ifdef(
#   CONFIG_BT_CTLR_CRYPTO
#   crypto.c
# )

zephyr_library_link_libraries(subsys__benchmark)

zephyr_include_directories(include .)