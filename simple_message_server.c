/**
 * @file simple_message_server.c
 * VCS TCP/IP Aufgabe 3: Implementierung Server
 *
 * @author Marco Judt (ic18b039@technikum-wien.at)
 * @author Andreas Hinterberger (ic18b008@technikum-wien.at)
 *
 * @git https://github.com/judtmarco/vcs_tcpip
 * @date 11/25/2019
 * @version FINAL
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
#define LISTEN_BACKLOG 10 /* Maximum length to which the queue of pending connections may grow */
#define MIN_PORTNUMBER 1023 /* If under 1024 (well-known ports) - bind() fails */
#define MAX_PORTNUMBER 65536
#define ERROR -1

/*
 * --------------------------------------------------------------- globals --
 */
const char *prog_name = ""; /* To save the program name and display it in usage and error function */
int socket_fd = 0; /* File descriptor for connection with client - global to access it in create and error function */

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
* \brief Main function of simple_message.server.c
*
* This is the start of the code. Commandline arguments are checked and parsed with getopt_long. Signal handler for child process
* is initialized and loop for incoming connections is done.
*
\param argc Contains number of arguments
\param argv An Array of char const pointer with the command line arguments
\return EXIT_SUCCESS If successfully completed
*/
int main (const int argc, char* const argv[])
{
    prog_name = argv[0];

    if (argc < 2)
    {
        usage();
        return EXIT_FAILURE;
    }

    // Parse command line arguments with getopt_long
    int opt = 0;
    long int port = 0;
    char *strtol_ptr = NULL;
    static struct option long_options[] = {
            {"port",    required_argument,  0,  'p' },
            {"help",    no_argument,        0,  'h' },
            {0,         0,                  0,   0 }
    };

    while ((opt = getopt_long(argc, argv, "p:h", long_options, NULL)) != ERROR) {
        switch (opt) {
            case 'p':
                errno = 0;
                port = strtol(optarg, &strtol_ptr, 10); /* Convert given port to long int */
                if (errno != 0) {
                    exit_on_error(errno, "strtol() failed");
                }
                /* Check if Port is in range and check the next value after numerical value (has to be \0) */
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
    memset (&sa, 0, sizeof(sa)); /* Set sa to 0 to not be undefined */
    sa.sa_handler = sigchild_handler; /* Configure signal handler with sigchild_handler to reap all dead processes */

    errno = 0;
    int ret_sigemptyset = sigemptyset(&sa.sa_mask); /* Initializes the signal set to empty */
    if (ret_sigemptyset == ERROR) {
        exit_on_error(errno, "sigemptyset() failed");
    }

    /* sa_flags is a set of flags which modify the behavior of the signal
     * SA_RESTART = Provide behavior compatible with BSD signal semantics by making certain system calls restartable across signals */
    sa.sa_flags = SA_RESTART;

    errno = 0;
    int ret_sigaction = sigaction(SIGCHLD, &sa, NULL); /* Change the action taken by process on receipt of a specific signal */
    if (ret_sigaction == ERROR) {
        exit_on_error(errno, "sigaction() failed");
    }

    // Loop for incoming connections from client
    socklen_t client_addr_size;
    struct sockaddr_storage client_addr; /* Store socket address information */
    int new_fd = 0;
    while (1) {
        client_addr_size = sizeof(client_addr);

        errno = 0;
        /* Extract the first connection request on the queue of pending connections for the listening socket
         * and create new connected socket*/
        new_fd = accept(socket_fd, (struct sockaddr *) &client_addr, &client_addr_size);
        if (new_fd == ERROR) {
            close (new_fd); /* Additionally close new connected socket */
            exit_on_error(errno, "accept() failed");
        }

        errno = 0;
        /* Create new process by duplicating the calling process */
        pid_t cpid = fork();
        if (cpid == ERROR) {
            close (new_fd); /* Additionally close new connected socket */
            exit_on_error(errno, "fork() failed");
        }

        // This is the child process
        if (cpid == 0) {
            errno = 0;
            int ret_close = close (socket_fd); /* Child has to close listening socket */
            if (ret_close == ERROR) {
                exit_on_error(errno, "close() listening socket failed");
            }

            // Create copy of new connected socket file descriptor using STDIN and STDOUT
            errno = 0;
            int ret_dup = dup2 (new_fd, STDIN_FILENO);
            if (ret_dup == ERROR) {
                close (new_fd); /* Additionally close new connected socket */
                exit_on_error(errno, "dup2() failed");
            }

            errno = 0;
            int ret_dup2 = dup2 (new_fd, STDOUT_FILENO);
            if (ret_dup2 == ERROR) {
                close (new_fd); /* Additionally close new connected socket */
                exit_on_error(errno, "dup2() failed");
            }

            // Execute the business logic
            errno = 0;
            int ret_execlp = execlp("simple_message_server_logic", "simple_message_server_logic", NULL);
            if (ret_execlp == ERROR)
            {
                close (new_fd); /* Additionally close new connected socket */
                exit_on_error(errno, "execlp() failed");
            }

            /* Close connected socket when done */
            ret_close = close (new_fd);
            if (ret_close == ERROR) {
                exit_on_error(errno, "close() connected socket failed");
            }
            exit(EXIT_SUCCESS);
        }
        // This is the parent process
        else {
            errno = 0;
            int ret_close = close (new_fd); /* Parent has to close connected socket */
            if (ret_close == ERROR) {
                exit_on_error(errno, "close() connected socket failed");
            }
        }
    }
    /* Never reach this */
    return EXIT_SUCCESS;
}

/**
* \brief Message for usage of simple_message_server
*
* Displays a message on how to use the program.
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
* \brief Error function for exiting the code
*
* Checks if the socket file descriptor is already created and closes it if so. Checks if there is an error code and prints
* error message with or without on stderr. Exits the program.
*
\param error Error code to display in the error message
\param message Message what went wrong
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
* \brief Create socket file descriptor for connections
*
* In this function an endpoint for communication is created and a file descriptor that refers to that endpoint is set up.
* First, addrinfo structs are loaded up with getaddrinfo. Then the endpoint is created and bound to the specified server port.
* Lastly, the socket is configured to be a server listening socket.
*
\param port The specified server port given with command line arguments
\return socket_fd The created socket file descriptor
*/
static int create_socket (char *port) {
    struct addrinfo hints;
    struct addrinfo *result;
    int ret_getaddrinfo, ret_bind, ret_listen, ret_sockopt;

    /* Load up address structs with getaddrinfo */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;    /* Allow only IPv4 */
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

    /* Create a socket */
    errno = 0;
    socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket_fd == ERROR) {
        exit_on_error(errno, "socket() failed");
    }

    /* // Bind socket to the specified port */
    errno = 0;
    ret_bind = bind(socket_fd, result->ai_addr, result->ai_addrlen);
    if (ret_bind != 0) {
        exit_on_error(errno, "bind() failed");
    }

    /* Set the socket options - SOL_SOCKET needs to be set to set options at the socket level;
     * SO_REUSEADDR allows reuse of local addresses*/
    errno = 0;
    int optval = 1;
    ret_sockopt = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (ret_sockopt == ERROR) {
        exit_on_error(errno, "setsockopt() failed");
    }

    /* Set socket_fd up to be a server (listening) socket */
    errno = 0;
    ret_listen = listen(socket_fd, LISTEN_BACKLOG);
    if (ret_listen != 0) {
        exit_on_error(errno, "listen() failed");
    }

    return socket_fd;
}

/**
* \brief Function to reap all dead child processes
*
* This function uses waitpid to reap all dead child processes. WNOHANG is used to return immediately if no child has exited.
*
\param sig Signal handler function needs this parameter
\return void
*/
static void sigchild_handler (int sig) {
    /* Wait for child process to change state */
    while (waitpid(ERROR, NULL, WNOHANG) > 0);
    sig = sig;
}

/*
 * =================================================================== eof ==
 */