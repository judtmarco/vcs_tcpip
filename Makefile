##
## @file Makefile
## BIC-3 Verteilte Systeme
## simple_message_client & simple_message_server
##
## Judt Marco: ic18b39@technikum-wien.at
## Hinterberger Andreas: ic18b008@technikum-wien.at
##
## Manuel:
## 		- "$ make" compile the c-files
##		- "$ make doc" creat doxygen document
##      - "$ make clean" remove object files 
##		- "$ make cleanexe" remove executeables

##
## ------------------------------------------------------------- variables --
##

CC=gcc
CFLAGS=-DDEBUG -Wall -pedantic -Werror -Wextra -Wstrict-prototypes -Wformat=2 -fno-common -ftrapv -g -O3 -std=gnu11
CP=cp
CD=cd
MV=mv
GREP=grep
DOXYGEN=doxygen
EXCLUDE_PATTERN=footrulewidth




##
## ----------------------------------------------------------------- rules --
##

%.o: %.c
	$(CC) $(CFLAGS) -c $<

##
## --------------------------------------------------------------- targets --
##

.PHONY: all
all: simple_message_server simple_message_client

simple_message_server: simple_message_server.o
	$(CC) $(CFLAGS) -o "$@" "$^"


simple_message_client: simple_message_client.o simple_message_client_commandline_handling.o
	$(CC) $(CFLAGS) -o "$@" "simple_message_client.o" "simple_message_client_commandline_handling.o"


clean:
	rm -rf *.a
	rm -rf *.o

cleanexe:
	rm -rf simple_message_client simple_message_server


doc: html pdf

.PHONY: html
html:
	$(DOXYGEN) doxygen.dcf

pdf: html
	$(CD) doc/pdf && \
	$(MV) refman.tex refman_save.tex && \
	$(GREP) -v $(EXCLUDE_PATTERN) refman_save.tex > refman.tex && \
	$(RM) refman_save.tex && \
	make && \
	$(MV) refman.pdf refman.save && \
	$(RM) *.pdf *.html *.tex *.aux *.sty *.log *.eps *.out *.ind *.idx \
	      *.ilg *.toc *.tps Makefile && \
	$(MV) refman.save refman.pdf

##
## ---------------------------------------------------------- dependencies --
##

##
## =================================================================== eof ==
##
