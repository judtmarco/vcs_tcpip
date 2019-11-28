/**
 *  @file simple_message_client.c
 *  TCP/IP project for BIC-3 Verteilte Systeme
 *
 *  @author Hinterberger Andreas <ic18b008@technikum-wien.at>
 *  @author Judt Marco <ic18b0xx@technikum-wien.at>
 *
 */

/*
 * -------------------------------------------------------- INCLUDE ---------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <memory.h>
#include <unistd.h>
//#include <zconf.h>

/*
 * -------------------------------------------------------- HEADER------------------------------------------------------
 */

#include "./simple_message_client_commandline_handling.h"

/*
 * -------------------------------------------------------- DEFINE -----------------------------------------------------
 */

#define BUF_SIZE 256

/*
 * -------------------------------------------------------- GLOBAL VARIABLES -------------------------------------------
 */

// This variable is just do get feedback from the cliet-application, when running on annuminas.

bool feedback = true;

/*
 * -------------------------------------------------------- PROTOTYPES -------------------------------------------------
 */

void parse_Command(int argc, const char *argv[], const char **server, const char **port, const char **user, const char **message, const char **img_url, int *verbose);
void print_Manuel (FILE *fp , const char *program_name , int exit_value);
int connect_With_Server(const char *server_address, const char *server_port);

/*
 * -------------------------------------------------------- MAIN -------------------------------------------------------
 */
int main (int argc, const char *argv []) {




}

/*
 * -------------------------------------------------------- FUNCTIONS --------------------------------------------------
 */

int connect_With_Server(const char *server_address, const char *server_port) {

    struct addrinfo hints;
    struct addrinfo *serverinfo;
    struct addrinfo *rp;
    int return_value;
    int socket_file_descriptor;

    memset(&hints, 0, sizeof(hints));    /* Fill all the memory of hints with zeros */
    hints.ai_family = AF_UNSPEC;               /* IPv4 & IPv6 are possible */
    hints.ai_socktype = SOCK_STREAM;           /* Define socket for TCP */
    hints.ai_protocol = 0;                     /* Define for the returned socket any possible protocol */
    hints.ai_flags = AF_UNSPEC;                /* Takes my IP Address */

    if (feedback) {
        printf("function: connect_With_Server | trying: running getaddrinfo\n");
    }

    /*
     * getaddrinfo gets the server address, the port and hints (in hints are all the importent information).
     * It writes the result as a pointer into the struct serverinfo.
     */
    if ((return_value = getaddrinfo(server_address, server_port, &hints, &serverinfo))!=0) {
        fprintf(stderr, "Failure in getaddrinfo: %s\n", gai_strerror(return_value));
        return -1;
    }

    if (feedback) {
        printf("function: connect_With_Server | getaddrinfo successful\nfunction: connect_With_Server | trying: looping throw all sockets\n");
    }

    /*
     * Loop throw all the informations provided in struct serverinfo. Until finding a possible socket, which dont get
     * -1 from the socket function. On this socket trying to do a positiv connect afterwards.
     */

    for (rp = serverinfo; rp != NULL; rp = rp->ai_next){

        if ((socket_file_descriptor = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {

            perror("client: socket");
            continue;
        }

        if (connect(socket_file_descriptor, rp->ai_addr, rp->ai_addrlen) == -1 ){

            close(socket_file_descriptor);
            perror("client: connected");
            continue;
        }

        break;
    }

    if (rp == NULL){

        fprintf(stderr, "client failed to connect to server");
        return -1;
    }

    return socket_file_descriptor;

}


void parse_Command (int argc, const char *argv[], const char **server, const char **port, const char **user, const char **message, const char **img_url, int *verbose){

    smc_parsecommandline (argc, argv, (smc_usagefunc_t) &print_Manuel, server, port, user, message, img_url, verbose);

    if ((*server == NULL)|| (*port == NULL)|| (*user == NULL)|| (*message == NULL)) {

        exit(EXIT_FAILURE);
    }
}


void print_Manuel (FILE *fp , const char *program_name , int exit_value) {

    fprintf(fp, "usage: %s optoins\n", program_name);
    fprintf(fp, "options:\n");
    fprintf(fp, "          -s, --server <server>   full qualified domain name or IP address of the server\n");
    fprintf(fp, "          -p, --port <port>       well-known port of the server [0..65535]\n");
    fprintf(fp, "          -u, --user <name>       name of the posting user\n");
    fprintf(fp, "          -i, --image <URL>       URL pointing to an image of the posting user\n");
    fprintf(fp, "          -m, --message <message> message to be added to the bulletin board\n");
    fprintf(fp, "          -v, --verbose           verbose output\n");
    fprintf(fp, "          -h, --help\n");

    exit(exit_value);
}
