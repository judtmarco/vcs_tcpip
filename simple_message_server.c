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
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/*
 * --------------------------------------------------------------- defines --
 */
#define LISTEN_BACKLOG 50
#define MAX_PORTNUMBER 65536
#define ERROR -1

/*
 * --------------------------------------------------------------- typedefs --
 */

/*
 * --------------------------------------------------------------- globals --
 */
const char *prog_name = "";
int socket_fd = 0;

/*
 * ------------------------------------------------------------- prototypes --
 */
static void usage(void);
static void error_exit (int error, char* message, int socket_fd);
static int create_socket (char *port);

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
    char *strtol_ptr = NULL;
    char portstring [6];
    static struct option long_options[] = {
            {"port",  required_argument, 0,  'p' },
            {"help",  no_argument,       0,  'h' },
            {NULL,  0, 0,  0 }
    };

    while ((opt = getopt_long(argc, argv, "p:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                errno = 0;
                port = strtol(optarg, &strtol_ptr, 10);
                if (errno != 0) {
                    error_exit(errno, "strtol() failed", 0);
                }
                if(port <= 0 || port >= MAX_PORTNUMBER || *strtol_ptr != '\0') {
                    error_exit(0, "port is invalid", 0);
                }
                strcpy (portstring, optarg);
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

    // Create and setup a socket
    socket_fd = create_socket(portstring);

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

/**
* \brief
*
*
\param
\return void
*/
static int create_socket (char *port) {
    struct addrinfo hints;
    struct addrinfo *result;
    int ret_getaddrinfo, ret_bind, ret_listen;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    errno = 0;
    ret_getaddrinfo = getaddrinfo(NULL, port, &hints, &result);
    if (ret_getaddrinfo != 0) {
        error_exit(errno, "getaddrinfo() failed", 0);
    }

    errno = 0;
    socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket_fd == ERROR) {
        error_exit(errno, "socket() failed", socket_fd);
    }

    errno = 0;
    ret_bind = bind(socket_fd, result->ai_addr, result->ai_addrlen);
    if (ret_bind != 0) {
        error_exit(errno, "bind() failed", socket_fd);
    }

    errno = 0;
    ret_listen = listen(socket_fd, LISTEN_BACKLOG);
    if (ret_listen != 0) {
        error_exit(errno, "listen() failed", socket_fd);
    }

    return socket_fd;
}

/*
 * =================================================================== eof ==
 */