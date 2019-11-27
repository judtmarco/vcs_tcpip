/**
 * @file simple_message_server.c
 * VCS TCP/IP Aufgabe 3: Implementierung Server
 *
 * @author Marco Judt (ic18b039@technikum-wien.at)
 * @author Andreas Hinterberger (ic18b008@technikum-wien.at)
 *
 * @git https://github.com/judtmarco/vcs_tcpip
 * @date 11/25/2019
 * @version 1.0
 */

/*
 * -------------------------------------------------------------- includes --
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/*
 * --------------------------------------------------------------- defines --
 */

/*
 * --------------------------------------------------------------- typedefs --
 */

/*
 * --------------------------------------------------------------- globals --
 */
const char *prog_name = "";

/*
 * ------------------------------------------------------------- prototypes --
 */
static void usage(void);


/*
 * ------------------------------------------------------------- functions --
 */

int main (const int argc, char* const argv[])
{
    prog_name = argv[0];

    if (argc < 2)
    {
        usage();
        // TODO: Close Sockets
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
* \brief Error message for usage of simple_message_server
*
* Prints an messege on how to use the program.
*
\param void
\return void
*/
static void usage(void)
{
    if (printf("usage: %s option\n"
               "options:\n"
               "\t-p, --port <port>\n"
               "\t-h, --help\n" , prog_name) < 0)
    {
        fprintf(stderr, "%s: printf(): %s\n", prog_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/*
 * =================================================================== eof ==
 */