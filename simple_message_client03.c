/**
 *  @file simple_message_client.c
 *  TCP/IP project for BIC-3 Verteilte Systeme
 *
 *  @author Hinterberger Andreas <ic18b008@technikum-wien.at>
 *  @author Judt Marco <ic18b0xx@technikum-wien.at>
 *
 *  @TODO Fehlerbehandlung!!! Fehlt noch komplett. Close, free, etc.
 *  @TODO Testcases anschauen
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
#include <errno.h>

/*
 * -------------------------------------------------------- HEADER------------------------------------------------------
 */

#include "./simple_message_client_commandline_handling.h"

/*
 * -------------------------------------------------------- DEFINE -----------------------------------------------------
 */

#define BUF_SIZE 512

/*
 * -------------------------------------------------------- GLOBAL VARIABLES -------------------------------------------
 */

// This variable is just to get feedback from the client-application, when running on annuminas.
bool feedback = true;

const char *prog_name = "";

static int verbose;

/*
 * -------------------------------------------------------- PROTOTYPES -------------------------------------------------
 */

void parse_Command(int argc, const char *argv[], const char **server, const char **port, const char **user, const char **message, const char **img_url, int *verbose);
void print_Manuel (FILE *fp , const char *program_name , int exit_value);
int connect_With_Server(const char *server_address, const char *server_port);
int send_Message_To_Server(int socket_file_descriptor, const char *message, const char *user, const char *img_url);
int receive_Message_From_Server(int socket_file_descriptor);
//static void exit_on_error (int error, char* message);
size_t read_File (FILE *fp,char *filename, size_t lenght);


/*
 * -------------------------------------------------------- MAIN -------------------------------------------------------
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

    if (argc < 2) {
        fprintf(stderr, "Not enough arguments\n");
        // Exit on error
    }

    const char *server = NULL;
    const char *port = NULL;
    const char *user = NULL;
    const char *message = NULL;
    const char *img_url = NULL;

    if (feedback){
        fprintf(stdout,"parsing the command\n");
    }
    parse_Command(argc,argv,&server, &port, &user, &message, &img_url, (int*)&verbose);

    int socket_w = 0;
    int socket_r = 0;

    if (feedback){
        fprintf(stdout,"connecting with server\n");
    }
    socket_w = connect_With_Server(server, port);

    socket_r = dup(socket_w);

    if (feedback){
        fprintf(stdout,"Sending message to server\n");
    }

    if (send_Message_To_Server(socket_w, message, user, img_url) != 0) {
        exit (-1);
        // Exit on error function
    }

    if (feedback){
        fprintf(stdout,"Receiving message from server\n");
    }

    if (receive_Message_From_Server(socket_r) != 0) {
        exit (-1);
        // Exit on error function
    }

    if (feedback){
        fprintf(stdout,"End program\n");
    }

    /*
    shutdown(fp_for_read, SHUT_RD);
    shutdown(fp_for_write, SHUT_WR);
    close(fp_for_write);
    close(fp_for_read); */

    return EXIT_SUCCESS;
}

/*
 * -------------------------------------------------------- FUNCTIONS --------------------------------------------------
 */

/**
* \brief
*
*
\param
\param
\return
*/
int receive_Message_From_Server (int socket_file_descriptor) {

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | trying opening reading descriptor\n");
    }

    // Open reading descriptor
    FILE *fp_for_read = fdopen(socket_file_descriptor, "r");
    if (fp_for_read == NULL) {
        fprintf(stderr, "Can not open file descriptor for reading");
        exit (-1);
        // Exit on error function
    }

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | successfully opened reading descriptor\n");
    }

    // Read "status=" from server response
    char *buffer = NULL;
    size_t length = 0;
    int status = 0;

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | trying reading a line from stream with getline\n");
    }

    errno = 0;
    if (getline(&buffer, &length, fp_for_read) == -1) {
        exit (-1);
        // Exit on error function
        // Check if errno != 0 or EOF
    }

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | successfully reading a line from stream with getline\n");
    }

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | trying reading status code from stream\n");
    }

    int ret_sscanf = sscanf(buffer, "status=%d", &status);
    if (ret_sscanf == 0 || ret_sscanf == EOF) {
        exit (-1);
        // Exit on error function
    }
    free(buffer);

    if (status != 0) {
        exit (-1);
        // Exit on error function with status code
    }

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | successfully reading status code from stream with status code=%d\n", status);
    }

    // Read "file=" from server response
    buffer = NULL;
    length = 0;
    char *fileName = NULL;

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | trying reading a line from stream with getline\n");
    }

    errno = 0;
    if (getline(&buffer, &length, fp_for_read) == -1) {
        exit (-1);
        // Exit on error function
        // Check if errno != 0 or EOF
    }

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | successfully reading a line from stream with getline\n");
    }

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | trying reading filename from stream\n");
    }

    fileName = malloc(sizeof(char) * strlen(buffer));
    if (fileName == NULL) {
        exit(-1);
        // Exit on error function
    }
    fileName[0] = '\0';

    ret_sscanf = sscanf(buffer, "file=%s", fileName);
    if (ret_sscanf == 0 || ret_sscanf == EOF) {
        exit (-1);
        // Exit on error function
    }
    free(buffer);

    if (strlen(fileName) == 0) {
        exit (-1);
        // Exit on error function
    }

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | successfully reading filename from stream with filename=%s\n", fileName);
    }

    // Read "len=" from server response
    buffer = NULL;
    length = 0;
    unsigned long fileLength = 0;

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | trying reading a line from stream with getline\n");
    }

    errno = 0;
    if (getline(&buffer, &length, fp_for_read) == -1) {
        exit (-1);
        // Exit on error function
        // Check if errno != 0 or EOF
    }

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | successfully reading a line from stream with getline\n");
    }

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | trying reading filelength from stream\n");
    }

    ret_sscanf = sscanf(buffer, "len=%lu", &fileLength);
    if (ret_sscanf == 0 || ret_sscanf == EOF) {
        exit (-1);
        // Exit on error function
    }
    free(buffer);

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | successfully reading filelength from stream with len=%ld\n", fileLength);
    }

    // Open file and write in it
    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | trying open outputfile\n");
    }

    FILE *outputFile = fopen(fileName, "w");
    if (outputFile == NULL) {
        exit (-1);
        // Exit on error function
    }

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | successfully opened outputfile\n");
    }

    if (read_File(fp_for_read,fileName,fileLength) < fileLength) {

        fprintf(stderr, "Reached EOF unexpacted");
        // Exit on error function
        exit(-1);

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
size_t read_File (FILE *fp ,char *filename, size_t lenght){

    bool error = false;
    size_t to_read = 0;
    size_t already_read = 0;
    size_t to_write = 0;
    size_t already_write = 0;
    char buffer [BUF_SIZE] = {'\0'};

    if (feedback){
        fprintf(stdout,"function read_File | trying opening filedecriptor for outputfile\n");
    }

    FILE *output_file = fopen(filename,"w");

    if (feedback){
        fprintf(stdout,"function read_File | successful opening filedecriptor for outputfile\ntrying ");
    }

    if (output_file == NULL) {

        error = true;
        fprintf(stderr, "Error while opening file descriptor");
        return -1;
    }

    while (lenght != 0){

        to_read = lenght > BUF_SIZE ? BUF_SIZE : lenght;

        already_read = fread(buffer, sizeof(char), to_read, fp);

        if (already_read == 0){

            fprintf(stderr, "Error not reading any characters");

            break;
        }

        fprintf(stdout,"in While for writing down the file");

        to_write = fwrite(buffer, sizeof(char), to_read, output_file);

        if (to_read != to_write){

            error = true;
            fprintf(stderr, "Error due not writing the same amount of characters as read");
            break;
        }

        already_write+=to_write;
        lenght-=to_read;
    }

    fclose(output_file);

    if (error) {

        return -1;

    } else {

        if (feedback) {
            fprintf(stdout,"Successfully writing file");
        }
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
int send_Message_To_Server(int socket_file_descriptor, const char *message, const char *user, const char *img_url){

    errno = 0;
    FILE *fp_for_write = fdopen(socket_file_descriptor,"w");
    if (fp_for_write == NULL) {
        exit (-1);
        // Exit on error function with errno
    }

    // Sending message without img-URL
    if (img_url == NULL){

        if (feedback) {
            fprintf(stdout,"function: send_Message_To_Server | trying: sending message without img-url to server\n");
        }

        // Hier gehört noch ein \n am ende vom string geschickt oder?
        if (fprintf(fp_for_write, "user=%s\n%s", user, message) < 0) {

            fprintf(stderr, "failed to write into file\n");
            fclose(fp_for_write);
            return -1;
            // Exit on error function
        }

        if (feedback) {
            fprintf(stdout, "function: send_Message_To_Server | successful sending message without img-url to server | trying fflush\n");
        }

        if (fflush(fp_for_write ) == EOF){

            fprintf(stderr, "failed to flush descriptor\n");
            fclose(fp_for_write);
            return -1;
        }

        if (feedback) {
            fprintf(stdout,"function: send_Message_To_Server | successful fflushh | trying shutdown\n");
        }

        if (shutdown(socket_file_descriptor,SHUT_WR) != 0){

            fprintf(stderr, "failed to shutdown descriptor\n");
            fclose(fp_for_write);
            return -1;
            // Exit on error function
        }

        if (feedback) {
            fprintf(stdout,"function: send_Message_To_Server | successful shutdown\n");
        }

        fclose(fp_for_write);

        if (feedback) {
            fprintf(stdout,"function: send_Message_To_Server | successful sending message without img-url to server\n");
        }

        // Sending message with img-URL
    } else {

        if (feedback) {
            fprintf(stdout,"function: send_Message_To_Server | trying: sending message with img-url to server\n");
        }

        // Hier gehört noch ein \n am ende vom string geschickt oder?
        if (fprintf(fp_for_write, "user=%s\nimg=%s\n%s", user, img_url, message) < 0) {

            fprintf(stderr, "failed to write into file\n");
            fclose(fp_for_write);
            return -1;
            // Exit on error function
        }

        if (feedback) {
            fprintf(stdout,"function: send_Message_To_Server | successful sending message with img-url to server | trying fflush\n");
        }

        if (fflush(fp_for_write) == EOF){

            fprintf(stderr, "failed to flush descriptor\n");
            fclose(fp_for_write);
            return -1;
            // Exit on error function
        }

        if (feedback) {
            fprintf(stdout,"function: send_Message_To_Server | successful fflushh | trying shutdown\n");
        }

        if (shutdown(socket_file_descriptor,SHUT_WR) != 0){

            fprintf(stderr, "failed to shutdown descriptor\n");
            fclose(fp_for_write);
            return -1;
            // Exit on error function
        }

        if (feedback) {
            fprintf(stdout,"function: send_Message_To_Server | successful shutdown\n");
        }

        fclose(fp_for_write);

        if (feedback) {
            fprintf(stdout,"function: send_Message_To_Server | seccussful sending message with img-url to server\n");
        }
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
int connect_With_Server(const char *server_address, const char *server_port) {

    struct addrinfo hints;
    struct addrinfo *serverinfo, *rp;
    int ret_getaddrinfo, socket_file_descriptor;

    memset(&hints, 0, sizeof(hints));          /* Fill all the memory of hints with zeros */
    hints.ai_family = AF_UNSPEC;               /* IPv4 & IPv6 are possible */
    hints.ai_socktype = SOCK_STREAM;           /* Define socket for TCP */
    hints.ai_protocol = 0;                     /* Define for the returned socket any possible protocol */

    // Hier vl AI_ADDRCONFIG bei ai_flags???
    hints.ai_flags = AF_UNSPEC;                /* Takes my IP Address */

    if (feedback) {
        fprintf(stdout,"function: connect_With_Server | trying: running getaddrinfo\n");
    }

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

    if (feedback) {
        fprintf(stdout,"function: connect_With_Server | getaddrinfo successful\nfunction: connect_With_Server | trying: looping throw all sockets\n");
    }

    /*
     * Loop throw all the informations provided in struct serverinfo. Until finding a possible socket, which dont get
     * -1 from the socket function. On this socket trying to do a positiv connect afterwards.
     */

    for (rp = serverinfo; rp != NULL; rp = rp->ai_next){

        /*if ((socket_file_descriptor = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }*/

        socket_file_descriptor = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (socket_file_descriptor == -1 ) {
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

    freeaddrinfo(serverinfo);

    if (rp == NULL){
        fprintf(stderr, "client failed to connect to server\n");
        return -1;
        // Exit on error function
    }

    if (feedback) {
        fprintf(stdout,"function: connect_With_Server | successful connected to Server\n");
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
void parse_Command (int argc, const char *argv[], const char **server, const char **port, const char **user, const char **message, const char **img_url, int *verbose){

    smc_parsecommandline (argc, argv, (smc_usagefunc_t) &print_Manuel, server, port, user, message, img_url, verbose);

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
/**
* \brief
*
*
\param
\param
\return
*/
/*
static void exit_on_error (int error, char* message) {

    if (error != 0) {
        fprintf(stderr, "%s: %s: %s\n", prog_name, message, strerror(error));
    }
    else {
        fprintf(stderr, "%s: %s\n", prog_name, message);
    }
    exit(EXIT_FAILURE);
}
*/