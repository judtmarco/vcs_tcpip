/**
 *  @file simple_message_client.c
 *  TCP/IP project for BIC-3 Verteilte Systeme
 *
 *  @author Hinterberger Andreas <ic18b008@technikum-wien.at>
 *  @author Judt Marco <ic18b0xx@technikum-wien.at>
 *
 *  @git https://github.com/judtmarco/vcs_tcpip
 *  @date 11/25/2019
 *  @version FINAL
 *
 *  @TODO Fehlerbehandlung!!! Fehlt noch komplett. Close, free, etc.
 *  @TODO Testcases anschauen
 *  @TODO Doxygen Comments, Comments
 */

/*
 * -------------------------------------------------------------- includes --
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <memory.h>
#include <unistd.h>
#include <errno.h>

/*
 * -------------------------------------------------------------- header --
 */
#include "simple_message_client_commandline_handling.h"

/*
 * -------------------------------------------------------------- defines --
 */
#define BUF_SIZE 512
#define ERROR -1

/*
 * -------------------------------------------------------------- globals --
 */
const char *prog_name = "";
static int verbose;
int socket_w = 0;
int socket_r = 0;

/*
 * -------------------------------------------------------------- prototypes --
 */
void parse_arguments(int argc, const char *argv[], const char **server, const char **port, const char **user, const char **message, const char **img_url, int *verbose);
void usage (FILE *fp, const char *prog_name, int exit_value);
static int connect_with_server(const char *server_address, const char *server_port);
static int send_message_to_server(int socket_file_descriptor, const char *message, const char *user, const char *img_url);
static int receive_message_from_server(int socket_file_descriptor);
static void exit_on_error (int error, char* message);
static size_t read_file (FILE *fp,char *filename, size_t lenght);

/*
 * -------------------------------------------------------------- functions --
 */
/**
* \brief
*
*
\param
\param
\return
*/
int main (int argc, const char *argv []) {

    prog_name = argv[0];

    const char *server = NULL;
    const char *port = NULL;
    const char *user = NULL;
    const char *message = NULL;
    const char *img_url = NULL;
    parse_arguments(argc,argv,&server, &port, &user, &message, &img_url, (int*)&verbose);

    socket_w = connect_with_server(server, port);
    if (socket_w == 0) {
        exit_on_error(0, "Connecting with server failed");
    }
    errno = 0;
    socket_r = dup(socket_w);
    if (socket_r == ERROR) {
        exit_on_error(errno, "Copying socket file descriptor failed");
    }

    if (send_message_to_server(socket_w, message, user, img_url) != 0) {
        exit_on_error(0, "Sending message to server failed");
    }

    if (receive_message_from_server(socket_r) != 0) {
        exit_on_error(0, "Receive message from server failed");
    }

    errno = 0;
    int ret_close = close (socket_w);
    if (ret_close == ERROR) {
        fprintf(stderr, "%s: close() socket_w failed: %s\n", prog_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    errno = 0;
    ret_close = close (socket_r);
    if (ret_close == ERROR) {
        fprintf(stderr, "%s: close() socket_r failed: %s\n", prog_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}

/**
* \brief
*
*
\param
\param
\return
*/
static int receive_message_from_server (int socket_file_descriptor) {

    // Open reading file descriptor
    errno = 0;
    FILE *fp_for_read = fdopen(socket_file_descriptor, "r");
    if (fp_for_read == NULL) {
        fclose(fp_for_read);
        exit_on_error(errno, "fdopen() failed");
    }

    // Read "status=" from server response
    char *buffer = NULL;
    size_t length = 0;
    int status = 0;

    errno = 0;
    if (getline(&buffer, &length, fp_for_read) == ERROR) {
        free(buffer);
        if (errno != 0) {
            fclose(fp_for_read);
            exit_on_error(errno, "Reading an entire line from stream failed");
        }
        // EOF found
        else {
            return 0;
        }
    }

    int ret_sscanf = sscanf(buffer, "status=%d", &status);
    if (ret_sscanf == 0 || ret_sscanf == EOF) {
        free(buffer);
        fclose(fp_for_read);
        exit_on_error(0, "sscanf() status= failed");
    }

    if (status != 0) {
        free(buffer);
        fclose(fp_for_read);
        close(socket_w);
        close(socket_r);
        fprintf(stderr, "%s: Failed with status code %d\n", prog_name, status);
        exit(status);
    }

    // Read file= and len= from server response
    char *fileName = NULL;
    unsigned long fileLength = 0;

    while (getline(&buffer, &length, fp_for_read) != ERROR) {

        fileName = malloc(sizeof(char) * strlen(buffer));
        if (fileName == NULL) {
            free(buffer);
            fclose(fp_for_read);
            exit_on_error(0, "malloc() failed");
        }
        memset(fileName, 0, sizeof(char));

        ret_sscanf = sscanf(buffer, "file=%s", fileName);
        if (ret_sscanf == 0 || ret_sscanf == EOF) {
            free(buffer);
            fclose(fp_for_read);
            exit_on_error(0, "sscanf() file= failed");
        }

        if (strlen(fileName) == 0) {
            free(buffer);
            fclose(fp_for_read);
            exit_on_error(0, "Reading filename from server response failed");
        }

        errno = 0;
        if (getline(&buffer, &length, fp_for_read) == ERROR) {
            free(buffer);
            if (errno != 0) {
                fclose(fp_for_read);
                exit_on_error(errno, "Reading an entire line from stream failed");
            }
            // EOF found
            else {
                return 0;
            }
        }

        ret_sscanf = sscanf(buffer, "len=%lu", &fileLength);
        if (ret_sscanf == 0 || ret_sscanf == EOF) {
            free(buffer);
            fclose(fp_for_read);
            exit_on_error(0, "sscanf() len= failed");
        }

        // Open file and write in it
        FILE *outputFile = fopen(fileName, "w");
        if (outputFile == NULL) {
            free(buffer);
            fclose(fp_for_read);
            exit_on_error(0, "fopen () outputFile failed");
        }

        if (read_file(fp_for_read, fileName, fileLength) < fileLength) {
            free(buffer);
            fclose(fp_for_read);
            exit_on_error(0, "Reached EOF unexpected");
        }
    }
    free(buffer);
    return 0;
}

/**
* \brief
*
*
\param
\param
\return
*/
static size_t read_file (FILE *fp ,char *filename, size_t lenght){

    bool error = false;
    size_t to_read = 0;
    size_t already_read = 0;
    size_t to_write = 0;
    size_t already_write = 0;
    char buffer [BUF_SIZE] = {'\0'};

    FILE *output_file = fopen(filename,"w");
    if (output_file == NULL) {
        error = true;
        fprintf(stderr, "Error while opening file descriptor");
        return -1;
        // Exit on error function
    }

    while (lenght != 0){

        if (lenght > BUF_SIZE) {
            to_read = BUF_SIZE;
        }
        else {
            to_read = lenght;
        }

        already_read = fread(buffer, sizeof(char), to_read, fp);
        if (already_read == 0){
            fprintf(stderr, "Error not reading any characters");
            break;
        }

        to_write = fwrite(buffer, sizeof(char), to_read, output_file);
        if (to_read != to_write){
            error = true;
            fprintf(stderr, "Error due not writing the same amount of characters as read");
            break;
        }

        already_write += to_write;
        lenght -= to_read;
    }

    fclose(output_file);

    if (error) {
        return -1;
    } else {
        return (already_write);
    }
}

/**
* \brief
*
*
\param
\param
\return
*/
static int send_message_to_server(int socket_file_descriptor, const char *message, const char *user, const char *img_url){

    errno = 0;
    FILE *fp_for_write = fdopen(socket_file_descriptor,"w");
    if (fp_for_write == NULL) {
        exit (-1);
        // Exit on error function with errno
    }

    // Sending message without img-URL
    if (img_url == NULL){
        if (fprintf(fp_for_write, "user=%s\n%s\n", user, message) < 0) {
            fprintf(stderr, "failed to write into file\n");
            fclose(fp_for_write);
            return -1;
            // Exit on error function
        }

        if (fflush(fp_for_write ) == EOF){
            fprintf(stderr, "failed to flush descriptor\n");
            fclose(fp_for_write);
            return -1;
            // Exit on error function
        }

        if (shutdown(socket_file_descriptor,SHUT_WR) != 0){
            fprintf(stderr, "failed to shutdown descriptor\n");
            fclose(fp_for_write);
            return -1;
            // Exit on error function
        }

        fclose(fp_for_write);

        // Sending message with img-URL
    } else {

        if (fprintf(fp_for_write, "user=%s\nimg=%s\n%s\n", user, img_url, message) < 0) {
            fprintf(stderr, "failed to write into file\n");
            fclose(fp_for_write);
            return -1;
            // Exit on error function
        }

        if (fflush(fp_for_write) == EOF){
            fprintf(stderr, "failed to flush descriptor\n");
            fclose(fp_for_write);
            return -1;
            // Exit on error function
        }

        if (shutdown(socket_file_descriptor,SHUT_WR) != 0){
            fprintf(stderr, "failed to shutdown descriptor\n");
            fclose(fp_for_write);
            return -1;
            // Exit on error function
        }

        fclose(fp_for_write);
    }
    return 0;
}

/**
* \brief
*
*
\param
\param
\return
*/
static int connect_with_server(const char *server_address, const char *server_port) {

    struct addrinfo hints;
    struct addrinfo *serverinfo, *rp;
    int ret_getaddrinfo, socket_file_descriptor;

    memset(&hints, 0, sizeof(hints));          /* Fill all the memory of hints with zeros */
    hints.ai_family = AF_UNSPEC;               /* IPv4 & IPv6 are possible */
    hints.ai_socktype = SOCK_STREAM;           /* Define socket for TCP */
    hints.ai_protocol = 0;                     /* Define for the returned socket any possible protocol */
    hints.ai_flags = AI_ADDRCONFIG;            /* Takes my IP Address */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    /*
     * getaddrinfo gets the server address, the port and hints (in hints are all the importent information).
     * It writes the result as a pointer into the struct serverinfo.
     */

    ret_getaddrinfo = getaddrinfo(server_address, server_port, &hints, &serverinfo);
    if (ret_getaddrinfo != 0) {
        fprintf(stderr, "Failure in getaddrinfo: %s\n", gai_strerror(ret_getaddrinfo));
        return -1;
        // Exit on error function
    }
    
    /*
     * Loop throw all the informations provided in struct serverinfo. Until finding a possible socket, which dont get
     * -1 from the socket function. On this socket trying to do a positiv connect afterwards.
     */
    for (rp = serverinfo; rp != NULL; rp = rp->ai_next){

        socket_file_descriptor = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (socket_file_descriptor == ERROR) {
            perror("client: socket");
            continue;
        }

        if (connect(socket_file_descriptor, rp->ai_addr, rp->ai_addrlen) == ERROR){
            close(socket_file_descriptor);
            perror("client: connected");
            continue;
        }
        break;
    }

    freeaddrinfo(serverinfo);

    if (rp == NULL){
        fprintf(stderr, "client failed to connect to server\n");
        return -1;
        // Exit on error function
    }
    
    return socket_file_descriptor;
}

/**
* \brief
*
*
\param
\param
\return
*/
void parse_arguments (int argc, const char *argv[], const char **server, const char **port, const char **user, const char **message, const char **img_url, int *verbose){

    smc_parsecommandline (argc, argv, (smc_usagefunc_t) &usage, server, port, user, message, img_url, verbose);

    if ((*server == NULL)|| (*port == NULL)|| (*user == NULL)|| (*message == NULL)) {
        exit(EXIT_FAILURE);
    }
}

/**
* \brief
*
*
\param
\param
\return
*/
void usage (FILE *fp, const char *prog_name, int exit_value) {
    fprintf(fp, "usage: %s options\n", prog_name);
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
/**
* \brief
*
*
\param
\param
\return
*/
static void exit_on_error (int error, char* message) {

    if (socket_w != 0) {
        close (socket_w);
    }
    if (socket_r != 0) {
        close (socket_r);
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