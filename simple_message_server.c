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
 * TODO: Doxygen comments and comment the code
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
#include <signal.h>
#include <sys/wait.h>

/*
 * --------------------------------------------------------------- defines --
 */
#define LISTEN_BACKLOG 30
#define MAX_PORTNUMBER 65536
#define MIN_PORTNUMBER 1023
#define ERROR -1

/*
 * --------------------------------------------------------------- globals --
 */
const char *prog_name = "";
int socket_fd = 0;

/*
 * ------------------------------------------------------------- prototypes --
 */
static void usage(void);
static void exit_on_error (int error, char* message);
static int create_socket (char *port);
static void sigchild_handler (int sig);

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
    static struct option long_options[] = {
            {"port",  required_argument, 0,  'p' },
            {"help",  no_argument,       0,  'h' },
            {NULL,  0, 0,  0 }
    };

    while ((opt = getopt_long(argc, argv, "p:h", long_options, NULL)) != ERROR) {
        switch (opt) {
            case 'p':
                errno = 0;
                port = strtol(optarg, &strtol_ptr, 10);
                if (errno != 0) {
                    exit_on_error(errno, "strtol() failed");
                }
                if(port <= MIN_PORTNUMBER || port >= MAX_PORTNUMBER || *strtol_ptr != '\0') {
                    exit_on_error(0, "port is invalid");
                }
                // Create and setup a socket
                socket_fd = create_socket(optarg);
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

    // Initializing signal handler for child process
    struct sigaction sa;
    sa.sa_handler = sigchild_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    errno = 0;
    int ret_sigaction = sigaction(SIGCHLD, &sa, NULL);
    if (ret_sigaction == ERROR) {
        exit_on_error(errno, "sigaction() failed");
    }

    // Loop for incoming connections from client
    socklen_t client_addr_size;
    struct sockaddr_storage client_addr;
    int new_fd = 0;
    while (1) {
        client_addr_size = sizeof(client_addr);

        errno = 0;
        new_fd = accept(socket_fd, (struct sockaddr *) &client_addr, &client_addr_size);
        if (new_fd == ERROR) {
            close (new_fd);
            exit_on_error(errno, "accept() failed");
        }

        errno = 0;
        pid_t cpid = fork();
        if (cpid == ERROR) {
            close (new_fd);
            exit_on_error(errno, "fork() failed");
        }

        // Child
        if (cpid == 0) {
            close (socket_fd);

            errno = 0;
            int ret_dup = dup2 (new_fd, STDIN_FILENO);
            if (ret_dup == ERROR) {
                close (new_fd);
                exit_on_error(errno, "dup2() failed");
            }

            errno = 0;
            int ret_dup2 = dup2 (new_fd, STDOUT_FILENO);
            if (ret_dup2 == ERROR) {
                close (new_fd);
                exit_on_error(errno, "dup2() failed");
            }

            errno = 0;
            int ret_execlp = execlp("simple_message_server_logic", "simple_message_server_logic", NULL);
            if (ret_execlp == ERROR)
            {
                close (new_fd);
                exit_on_error(errno, "execlp() failed");
            }

            close (new_fd);
            exit(EXIT_SUCCESS);
        }
        // Parent
        else {
            close (new_fd);
        }
    }
    /* Never reach this */
    return EXIT_SUCCESS;
}

/**
* \brief Error message for usage of simple_message_server
*
* Prints an message on how to use the program.
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
        exit_on_error(0, "printf() failed");
    }
}

/**
* \brief
*
*
\param
\return void
*/
static void exit_on_error (int error, char* message) {
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
\return
*/
static int create_socket (char *port) {
    struct addrinfo hints;
    struct addrinfo *result;
    int ret_getaddrinfo, ret_bind, ret_listen;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Bytestream socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    errno = 0;
    ret_getaddrinfo = getaddrinfo(NULL, port, &hints, &result);
    if (ret_getaddrinfo != 0) {
        exit_on_error(errno, "getaddrinfo() failed");
    }

    errno = 0;
    socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket_fd == ERROR) {
        exit_on_error(errno, "socket() failed");
    }

    errno = 0;
    ret_bind = bind(socket_fd, result->ai_addr, result->ai_addrlen);
    if (ret_bind != 0) {
        exit_on_error(errno, "bind() failed");
    }

    errno = 0;
    int yes = 1;
    int ret_sockopt = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (ret_sockopt == ERROR) {
        exit_on_error(errno, "setsockopt() failed");
    }

    errno = 0;
    ret_listen = listen(socket_fd, LISTEN_BACKLOG);
    if (ret_listen != 0) {
        exit_on_error(errno, "listen() failed");
    }

    return socket_fd;
}

/**
* \brief
*
*
\param
\return void
*/
static void sigchild_handler (int sig) {
    sig = sig;

    int pid = waitpid(ERROR, NULL, WNOHANG);
    while (pid > 0) {
        pid = waitpid(ERROR, NULL, WNOHANG);
    };
}

/*
 * =================================================================== eof ==
 */