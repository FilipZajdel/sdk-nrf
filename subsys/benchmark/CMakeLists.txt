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
  zigbee_benchmark.c
  zigbee_commands.c
  zigbee_protocol.c
  zigbee_bm_cli.c
  zigbee_timer.c
  zigbee_zcl_commands.c
)


zephyr_include_directories(include .)