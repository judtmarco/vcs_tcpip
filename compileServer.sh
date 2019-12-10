#!/bin/bash

gcc -Wall -Werror -Wextra -Wstrict-prototypes -Wformat=2 -pedantic -fno-common -ftrapv -O3 -g -std=gnu11 simple_message_server.c -o simple_message_server
