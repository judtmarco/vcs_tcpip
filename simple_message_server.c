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
 *
 */

/*
 * -------------------------------------------------------------- includes --
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

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
static void error_exit (int error, char* message, int socket_fd);

/*
 * ------------------------------------------------------------- functions --
 */

/**
* \brief
*
*
\param
\return
*/
int main (const int argc, char* const argv[])
{
    prog_name = argv[0];

    if (argc < 2)
    {
        usage();
        return EXIT_FAILURE;
    }

    // Parse command line arguments with getopt
    int opt = 0;
    long int port = 0;
    char *portstring_ptr = NULL;
    static struct option long_options[] = {
            {"port",  required_argument, 0,  'p' },
            {"help",  no_argument,       0,  'h' },
            {NULL,  0, 0,  0 }
    };

    while ((opt = getopt_long(argc, argv, "p:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                errno = 0;
                port = strtol(optarg, &portstring_ptr, 10);
                if (errno != 0) {
                    error_exit(errno, "strtol() failed", 0);
                }
                if(port <= 0 || port >= 65536 || *portstring_ptr != '\0') {
                    error_exit(0, "port is invalid", 0);
                }
                printf("You have port: %ld\n", port);
                break;
            case 'h':
                usage();
                exit(EXIT_FAILURE);
                break;
            default:
                usage();
                exit(EXIT_FAILURE);
                break;
        }
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
        error_exit(errno, "printf() failed", 0);
    }
}

/**
* \brief
*
*
\param
\return void
*/
static void error_exit (int error, char* message, int socket_fd) {
    if (socket_fd != 0) {
        close(socket_fd);
    }

    if (error != 0) {
        fprintf(stderr, "%s: %s: %s\n", prog_name, message, strerror(error));
    }
    else {
        fprintf(stderr, "%s: %s\n", prog_name, message);
    }
    exit(EXIT_FAILURE);
}

/*
 * =================================================================== eof ==
 */