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
int send_Message_To_Server(int socket_file_descriptor, const char *message, const char *user, const char *img_url);
int read_Message_From_Server(int socket_file_descriptor);


/*
 * -------------------------------------------------------- MAIN -------------------------------------------------------
 */

int main (int argc, const char *argv []) {

  




}

/*
 * -------------------------------------------------------- FUNCTIONS --------------------------------------------------
 */

int read_Message_From_Server (int socket_file_descriptor){

    char *buffer = calloc(1 ,BUF_SIZE);
    long offset = 0;



}




int send_Message_To_Server(int socket_file_descriptor, const char *message, const char *user, const char *img_url){

    size_t length_user_segment = strlen(user) + 6;                  /* get the size of the user, +6 for "user=" */
    char *user_segment = malloc(length_user_segment + 1);      /* allocate the size of user, +1 for nullcharacter */

    size_t length_imgurl_segment;
    char *imgurl_segment;

    size_t length_of_message = strlen(message);

    // ----------------------------------------------------- USER
    if (feedback) {
        printf("function: send_Message_To_Server | trying: sending username to server\n");
    }

    if (sprintf(user_segment, "user=%s\n", user) < 0) {               /* writing the user-info in the right format into the string user segment */

        fprintf(stderr, "failed to write into usersegment\n");
        free(user_segment);
        return -1;
    }

    if (send(socket_file_descriptor, user_segment, length_user_segment, 0) == -1){

        fprintf(stderr, "failed sending username to server\n");
        free(user_segment);
        return -1;
    }

    free(user_segment);

    // ----------------------------------------------------IMAGE-URL
    if (feedback) {
        printf("function: send_Message_To_Server | successful sending username to server\n");
    }

    if (img_url != NULL){

        if (feedback) {
            printf("function: send_Message_To_Server | trying: sending to image url to server\n");
        }

        length_imgurl_segment = strlen(img_url) + 5;                                 /* get the size of the img_url, +5 for "user=" */
        imgurl_segment = malloc(length_imgurl_segment + 1);                     /* allocate the size of img_url, +1 for nullcharacter */

        if (sprintf(imgurl_segment, "img=%s\n", img_url) < 0) {               /* writing the image-url in the right format into the string imgurl_segment */

            fprintf(stderr, "failed to write into image-url-segment\n");
            free(imgurl_segment);
            return -1;
        }

        if (send(socket_file_descriptor, imgurl_segment, length_imgurl_segment, 0) == -1){

            fprintf(stderr, "failed sending image-Url to server\n");
            free(imgurl_segment);
            return -1;
        }

        free(imgurl_segment);

        if (feedback) {
            printf("function: send_Message_To_Server | sending image url to server successful\n trying sending message");
        }
    }

    // -------------------------------------------------MESSAGE

    if (send(socket_file_descriptor, message, length_of_message, 0) == -1){

        fprintf(stderr, "failed sending message to server\n");
        free(imgurl_segment);
        return -1;
    }

    if (feedback) {
        printf("function: send_Message_To_Server | successful sending message to server\n");
    }
}

int connect_With_Server(const char *server_address, const char *server_port) {

    struct addrinfo hints;
    struct addrinfo *serverinfo;
    struct addrinfo *rp;
    int return_value;
    int socket_file_descriptor;

    memset(&hints, 0, sizeof(hints));       /* Fill all the memory of hints with zeros */
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

        fprintf(stderr, "client failed to connect to server\n");
        return -1;
    }

    if (feedback) {
        printf("function: connect_With_Server | seccussful connected to Server\n");
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
