#!/bin/bash

gcc -Wall -Werror -Wextra -Wstrict-prototypes -Wformat=2 -pedantic -fno-common -ftrapv -O3 -g -std=gnu11 simple_message_client02.c simple_message_client_commandline_handling.c -o simple_message_client
