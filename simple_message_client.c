/**
 * \file simple_message_client.c
 * TCP/IP project for BIC-3 Verteilte Systeme
 *
 * @author Hinterberger Andreas <ic18b008@technikum-wien.at>
 * @author Judt Marco <ic18b039@technikum-wien.at>
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
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
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
int socket_w = 0;
int socket_r = 0;

/*
 * -------------------------------------------------------------- prototypes --
 */
void parse_arguments(int argc, const char *argv[], const char **server, const char **port, const char **user, const char **message, const char **img_url, int *verbose);
void usage (FILE *fp, const char *prog_name, int exit_value);
static int connect_with_server(const char *server_address, const char *server_port);
static int send_message_to_server(const char *message, const char *user, const char *img_url);
static int receive_message_from_server(void);
static void exit_on_error (int error, char* message);
static size_t read_file (FILE *fp, char *filename, size_t length);

/*
 * -------------------------------------------------------------- functions --
 */
/**
* \brief Main function defines the function flow
*
* Main function makes a sequential call of the functions: parse_arguments, connect_with_server,
* send_message_to_server and receive_message_from_server.
\param argc number of arguments
\param argv vector with the given arguments
\return EXIT_SUCCESS if the program finishes normal
*/
int main (int argc, const char *argv []) {

    prog_name = argv[0];

    const char *server = NULL;
    const char *port = NULL;
    const char *user = NULL;
    const char *message = NULL;
    const char *img_url = NULL;
    int verbose;

    parse_arguments(argc,argv,&server, &port, &user, &message, &img_url, &verbose);

    if (connect_with_server(server, port) != 0) {
        exit_on_error(0, "Connecting with server failed");
    }

    errno = 0;
    socket_r = dup(socket_w);
    if (socket_r == ERROR) {
        exit_on_error(errno, "Copying socket file descriptor failed");
    }

    if (send_message_to_server(message, user, img_url) != 0) {
        exit_on_error(0, "Sending message to server failed");
    }

    if (receive_message_from_server() != 0) {
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
* \brief Handle the received message from the server
*
* Function opens file descriptor for reading the responses from file.
* Reading in status, file and length from server.
* Gives the length and the filename to the function read_file.
*
\param void
\return 0 when successful reading responses from server
*/
static int receive_message_from_server (void) {

    // Open reading file descriptor
    errno = 0;
    FILE *fp_for_read = fdopen(socket_r, "r");
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

    // Read file= and len= from server response and call function read_file
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
* \brief Reading and writing down the file
*
* Function open the file where to write down the file given by the server based on the filename.
* In while reading in the file and write it into the outputfile, step by step.
* The while is needed to calculate if the read bytes matches the given bytes by the server.
*
\param *fp is the file descriptor for reading out of the file
\param *filename is the given name for the file which will be written down
\param length is the size of the file in byte which will be read from the server and then written down
\return -1 if failure occurred or the read file length is not the same as given from parameter length
\return the written bytes, which should be the same as given by parameter length
*/
static size_t read_file (FILE *fp ,char *filename, size_t length){

    size_t to_read = 0;
    size_t already_read = 0;
    size_t to_write = 0;
    size_t already_write = 0;
    char buffer [BUF_SIZE] = {'\0'};

    errno = 0;
    // Open the output-file with the given name
    FILE *output_file = fopen(filename,"w");
    if (output_file == NULL) {
        fclose(output_file);
        exit_on_error(errno, "fopen() output_file failed");
    }
    // Read and write step by step through the file, based on the given length 
    while (length != 0){
        if (length > BUF_SIZE) {
            to_read = BUF_SIZE;
        }
        else {
            to_read = length;
        }

        already_read = fread(buffer, sizeof(char), to_read, fp);
        if (already_read == 0){
            fclose(output_file);
            exit_on_error(0, "fread() failed");
        }

        to_write = fwrite(buffer, sizeof(char), to_read, output_file);
        if (to_read != to_write){
            fclose(output_file);
            exit_on_error(0, "fwrite() wrote wrong amount of characters");
        }
        already_write += to_write;
        length -= to_read;
    }
    fclose(output_file);
    return (already_write);
}

/**
* \brief Sending a message to the server
*
* Function opens a writing file descriptor based on the given socket.
* After that the function distinguish between sending an image-URL or not.
* The writing to the server is done via fprintf.
* Afterwords the descriptor gets flushed, shutdown for writing and closed.
*
\param *message includes the message provided by the user
\param *user is the username provided by the user
\param *img_url includes the URL to a image which is provided by the user
\return 0 on successful sending message to server
\return -1 on fail sending message to server
*/
static int send_message_to_server(const char *message, const char *user, const char *img_url){

    errno = 0;
    FILE *fp_for_write = fdopen(socket_w,"w");
    if (fp_for_write == NULL) {
        fclose(fp_for_write);
        exit_on_error(errno, "fdopen() failed");
    }

    // Sending message without img-URL
    if (img_url == NULL){
        if (fprintf(fp_for_write, "user=%s\n%s\n", user, message) < 0) {
            fclose(fp_for_write);
            exit_on_error(0, "fprintf() failed");
        }

        errno = 0;
        if (fflush(fp_for_write) == EOF){
            fclose(fp_for_write);
            exit_on_error(errno, "fflush() failed");
        }

        errno = 0;
        if (shutdown(socket_w,SHUT_WR) == ERROR){
            fclose(fp_for_write);
            exit_on_error(errno, "shutdown() failed");
        }
        fclose(fp_for_write);

        // Sending message with img-URL
    }
    else {
        if (fprintf(fp_for_write, "user=%s\nimg=%s\n%s\n", user, img_url, message) < 0) {
            fclose(fp_for_write);
            exit_on_error(0, "fprintf() failed");
        }

        errno = 0;
        if (fflush(fp_for_write) == EOF){
            fclose(fp_for_write);
            exit_on_error(errno, "fflush() failed");
        }

        errno = 0;
        if (shutdown(socket_w,SHUT_WR) != 0){
            fclose(fp_for_write);
            exit_on_error(errno, "shutdown() failed");
        }
        fclose(fp_for_write);
    }
    return 0;
}

/**
* \brief Function connects with and handles the socket connection to the server
*
* Heart of the function is getaddressinfo which writes the information into the struct serverinfo.
* The struct hints the configurations based on the programmer.
* Afterwords loop through struct rp=serverinfo which is needed to find a socket between server and client.
*
\param server_address provides the address of the server given by the user
\param server_port provides the port of the server given by the user
\return 0 If successfully connected with a server
\return -1 If failure occurred
*/
static int connect_with_server(const char *server_address, const char *server_port) {

    struct addrinfo hints;
    struct addrinfo *serverinfo, *rp;

    memset(&hints, 0, sizeof(hints));          /* Fill all the memory of hints with zeros */
    hints.ai_family = AF_UNSPEC;               /* IPv4 & IPv6 are possible */
    hints.ai_socktype = SOCK_STREAM;           /* Define socket for TCP */
    hints.ai_protocol = 0;                     /* Define for the returned socket any possible protocol */
    hints.ai_flags = AI_ADDRCONFIG;            /* Define if host offers at least one IPv4 or/and IPv6 address */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    /*
     * getaddrinfo gets the server address, the port and hints (which includes the parameters set above).
     * The result get into the struct: serverinfo.
     */
    errno = 0;
    int ret_getaddrinfo = getaddrinfo(server_address, server_port, &hints, &serverinfo);
    if (ret_getaddrinfo != 0) {
        exit_on_error(errno, "getaddrinfo() failed");
    }

    /*
     * Loop throw all the informations provided in struct serverinfo. Until finding a possible socket, which dont get
     * -1 from the socket function. On this socket trying to do a positiv connect afterwards.
     */
    for (rp = serverinfo; rp != NULL; rp = rp->ai_next){

        socket_w = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (socket_w == ERROR) {
            continue;
        }

        if (connect(socket_w, rp->ai_addr, rp->ai_addrlen) == ERROR){
            close(socket_w);
            continue;
        }
        break;
    }

    freeaddrinfo(serverinfo);

    if (rp == NULL){
        exit_on_error(0, "Connection to server failed");
    }

    return 0;
}

/**
* \brief Parsing the commandline
*
* Parsing the commandline for all the needed parameters in simple_message_client.
* (This function is given from simple_message_client_commandline_handler.h)
* EXIT_FAILURE will get triggered if server, port, message or user are NULL.
*
\param argc argument counter
\param *argv vector of given arguments
\param **server address of the server
\param **port port of the server to communicate to
\param **user user who writes the message
\param **message message written to the bulletinboard
\param **img_url url to image which should be displayed on the bulletinboard
\param *verbose printing out messages what is currently happening in the program (not supported)
\return void
*/

void parse_arguments (int argc, const char *argv[], const char **server, const char **port, const char **user, const char **message, const char **img_url, int *verbose){

    smc_parsecommandline (argc, argv, (smc_usagefunc_t) &usage, server, port, user, message, img_url, verbose);

    if ((*server == NULL)|| (*port == NULL)|| (*user == NULL)|| (*message == NULL)) {
        exit(EXIT_FAILURE);
    }
}

/**
* \brief Print out help informations for user
*
* Function get called if a failure of smc_parsecommandline occurres.
*
*
\param *fp defines the stream where the usage information should be written
\param *prog_name defines the name of the executable,
\param exit_value given exit code for terminating program
\return void
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
* \brief Printing error information
*
* Function prints out error information based on the given information.
* Closes the sockets and ends the program.
*
\param error Error code to display in the error message
\param message Message with information on what went wrong
\return EXIT_FAILURE since this function gets only called if an error occurred
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
