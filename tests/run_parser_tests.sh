#!/bin/bash

# ANSI Color codes
RESET='\033[0m'
MAGENTA='\033[35m'

echo -e "\n${MAGENTA}--- STARTING CONFIG PARSER TESTS (VALID) ---${RESET}"
for conf in tests/valid_configs/*.conf; do
    ./test_config_parser "$conf"
done

echo -e "\n${MAGENTA}--- STARTING CONFIG PARSER TESTS (ERRORS) ---${RESET}"
for conf in tests/error_configs/*.conf; do
    ./test_config_parser "$conf"
done

echo -e "\n\033[32m--- ALL CONFIG PARSER TESTS COMPLETED ---\033[0m\n"
