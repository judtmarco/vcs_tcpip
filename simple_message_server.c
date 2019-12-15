/**
 * @file simple_message_server.c
 * TCP/IP project for BIC-3 Verteilte Systeme
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
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>

/*
 * --------------------------------------------------------------- defines --
 */
#define LISTEN_BACKLOG 10 /* Maximum length to which the queue of pending connections may grow */
#define MAX_PORTNUMBER 65535
#define ERROR -1

/*
 * --------------------------------------------------------------- globals --
 */
const char *prog_name = ""; /* To save the program name and display it in usage and error function */
int listening_socket = 0; /* File descriptor for connection with client - global to access it in create and error function */

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
* is initialized and loop for incoming connections is handled.
*
\param argc Contains number of arguments
\param argv An Array of char const pointer with the command line arguments
\return EXIT_SUCCESS If successfully completed
*/
int main (const int argc, char* const argv[])
{
    prog_name = argv[0];

    if (argc < 2) {
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
                port = strtol(optarg, &strtol_ptr, 10); /* Convert given port to long int */
                if (errno != 0) {
                    exit_on_error(errno, "strtol() failed");
                }
                /* Check if port is in range and check the next value after numerical value (has to be \0) */
                if(port < 0 || port > MAX_PORTNUMBER || *strtol_ptr != '\0') {
                    exit_on_error(0, "port is invalid");
                }
                // Create and setup a socket
                if (create_socket(optarg) != 0) {
                    exit_on_error(0, "Creating socket failed");
                }
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

    // Manipulate POSIX signal sets
    errno = 0;
    int ret_sigemptyset = sigemptyset(&sa.sa_mask); /* Initializes the signal set to empty */
    if (ret_sigemptyset == ERROR) {
        exit_on_error(errno, "sigemptyset() failed");
    }

    /* sa_flags is a set of flags which modify the behavior of the signal
     * SA_RESTART = Provide behavior compatible with BSD signal semantics by making certain system calls restartable across signals */
    sa.sa_flags = SA_RESTART; /* Necessary when establishing a signal handler */

    errno = 0;
    int ret_sigaction = sigaction(SIGCHLD, &sa, NULL); /* Change the action taken by process on receipt of a specific signal */
    if (ret_sigaction == ERROR) {
        exit_on_error(errno, "sigaction() failed");
    }

    // Loop for incoming connections from client
    socklen_t client_addr_size;
    struct sockaddr_storage client_addr; /* Store socket address information */
    int connected_socket = 0;
    while (1) {
        client_addr_size = sizeof(client_addr);

        errno = 0;
        /* Extract the first connection request on the queue of pending connections for the listening socket
         * and create new connected socket*/
        connected_socket = accept(listening_socket, (struct sockaddr *) &client_addr, &client_addr_size);
        if (connected_socket == ERROR) {
            close (connected_socket); /* Additionally close new connected socket */
            exit_on_error(errno, "accept() failed");
        }

        errno = 0;
        /* Create new process by duplicating the calling process */
        pid_t cpid = fork();
        if (cpid == ERROR) {
            close (connected_socket); /* Additionally close new connected socket */
            exit_on_error(errno, "fork() failed");
        }

        // This is the child process
        if (cpid == 0) {
            errno = 0;
            int ret_close = close (listening_socket); /* Child has to close listening socket */
            if (ret_close == ERROR) {
                exit_on_error(errno, "close() listening socket failed");
            }

            /* Create copy of new connected socket file descriptor using STDIN and STDOUT */
            errno = 0;
            int ret_dup = dup2 (connected_socket, STDIN_FILENO);
            if (ret_dup == ERROR) {
                close (connected_socket); /* Additionally close new connected socket */
                exit_on_error(errno, "dup2() failed");
            }

            errno = 0;
            int ret_dup2 = dup2 (connected_socket, STDOUT_FILENO);
            if (ret_dup2 == ERROR) {
                close (connected_socket); /* Additionally close new connected socket */
                exit_on_error(errno, "dup2() failed");
            }

            /* Execute the business logic */
            errno = 0;
            int ret_execlp = execl("simple_message_server_logic", "simple_message_server_logic", NULL);
            if (ret_execlp == ERROR)
            {
                close (connected_socket); /* Additionally close new connected socket */
                exit_on_error(errno, "execlp() failed");
            }

            /* Close connected socket when done */
            ret_close = close (connected_socket);
            if (ret_close == ERROR) {
                exit_on_error(errno, "close() connected socket failed");
            }
            exit(EXIT_SUCCESS);
        }
        // This is the parent process
        else {
            errno = 0;
            int ret_close = close (connected_socket); /* Parent has to close connected socket */
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
\param message Message with information on what went wrong
\return void
*/
static void exit_on_error (int error, char* message) {
    if (listening_socket != 0) {
        close(listening_socket);
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
\return 0 If Successfully created a socket
*/
static int create_socket (char *port) {
    struct addrinfo hints, *result, *rp;
    int ret_getaddrinfo, ret_listen, ret_sockopt;

    /* Load up address structs with getaddrinfo */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;    /* Allow only IPv4 */
    hints.ai_socktype = SOCK_STREAM; /* Bytestream socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address to bind socket*/
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    errno = 0;
    ret_getaddrinfo = getaddrinfo(NULL, port, &hints, &result);
    if (ret_getaddrinfo != 0) {
        exit_on_error(errno, "getaddrinfo() failed");
    }

    /* Given the port getaddrinfo() returns a list of address structures.
       Try each address until we successfully bind.
       If socket (or bind) fails, we close the socket and try the next address.*/

    /* Create a socket */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        listening_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (listening_socket == ERROR) {
            continue;
        }

        /* Set the socket options
         * SOL_SOCKET needs to be set to manipulate options at the sockets API level;
         * SO_REUSEADDR allows reuse of local addresses: Allows other sockets to bind to this port; Avoids "Address already in use" error */
        errno = 0;
        int optval = 1;
        ret_sockopt = setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        if (ret_sockopt == ERROR) {
            exit_on_error(errno, "setsockopt() failed");
        }

        /* Bind socket to the specified port */
        if (bind(listening_socket, result->ai_addr, result->ai_addrlen) == 0) {
            break; // Success
        }
        close(listening_socket);
    }

    freeaddrinfo(result);           /* No longer needed */
    if (rp == NULL) {
        exit_on_error(0, "No address succeeded");
    }

    /* Set listening_socket up to be a passive server (listening) socket */
    errno = 0;
    ret_listen = listen(listening_socket, LISTEN_BACKLOG);
    if (ret_listen == ERROR) {
        exit_on_error(errno, "listen() failed");
    }
    return 0;
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
