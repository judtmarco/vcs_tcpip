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

FILE *fp_for_write;
FILE *fp_for_read;

const char *prog_name = "";

/*
 * -------------------------------------------------------- PROTOTYPES -------------------------------------------------
 */

void parse_Command(int argc, const char *argv[], const char **server, const char **port, const char **user, const char **message, const char **img_url, int *verbose);
void print_Manuel (FILE *fp , const char *program_name , int exit_value);
int connect_With_Server(const char *server_address, const char *server_port);
int send_Message_To_Server(int socket_file_descriptor, const char *message, const char *user, const char *img_url);
void receive_Message_From_Server(int socket_file_descriptor);
static void exit_on_error (int error, char* message);


/*
 * -------------------------------------------------------- MAIN -------------------------------------------------------
 */

int main (int argc, const char *argv []) {
    prog_name = argv[0];
    return EXIT_SUCCESS;
}

/*
 * -------------------------------------------------------- FUNCTIONS --------------------------------------------------
 */

void receive_Message_From_Server (int socket_file_descriptor){

    fp_for_read = fdopen(socket_file_descriptor,"r");
    if (fp_for_read == NULL) {
        // Close and exit on error
    }

    // Search "status=" in response from server
    char *lines = NULL;
    size_t lineSize = 0;

    // Read an entire line from fp_for_read
    errno = 0;
    int ret_getline = getline (&lines, &lineSize, fp_for_read);
    if (ret_getline == -1) {
        free(lines);
        if (errno != 0) {
            // Close and exit on error
        }
        else {
            // EOF
        }
    }

    // Read formatted input from a string
    int ret_sscanf = sscanf (lines, "status=%d", status);
    if (ret_sscanf == 0 || ret_sscanf == EOF) {
        free(lines);
        // Close and exit on error
    }

    if (status != 0) {
        // Close and exit on error with error code from status
    }

    // Search "file=" in response from server
    *lines = NULL;
    lineSize = 0;
    char *fileName = NULL;

    // Read an entire line from fp_for_read
    errno = 0;
    ret_getline = getline (&lines, &lineSize, fp_for_read);
    if (ret_getline == -1) {
        free(lines);
        if (errno != 0) {
            // Close and exit on error
        }
        else {
            // EOF
        }
    }

    fileName = malloc (sizeof(char) * strlen(lines));
    if (fileName == NULL) {
        free(lines);
        // Close and exit on error
    }

    ret_sscanf = sscanf (lines, "file=%s", fileName);
    if (ret_sscanf == 0 || ret_sscanf == EOF || (strlen(fileName) == 0)) {
        free(lines);
        free(fileName);
        // Close and exit on error
    }

    // Read the length from the given file

    *lines = NULL;
    lineSize = 0;
    char *fileLenght = NULL;

    errno = 0;
    ret_getline = getline(&lines, &lineSize,fp_for_read);

    if (ret_getline == -1) {
        free(lines);
        if (errno != 0) {
            // Close and exit on error
        }
        else {
            // EOF
        }
    }

    fileLenght = malloc (sizeof(char) * strlen(lines));
    if (fileLenght== NULL) {
        free(lines);
        // Close and exit on error
    }

    ret_sscanf = sscanf (lines, "len=%s", fileLenght);
    if (ret_sscanf == 0 || ret_sscanf == EOF || (strlen(fileLenght) == 0)) {
        free(lines);
        free(fileLenght);
        // Close and exit on error
    }

    long val = 0;
    char *test;       // hier sollte der char anteil gespeichert werden

    val = strtol(fileLenght, *test, 32);
    int lenght_of_file = val +1;                                          // falls eine Komazahl raus kommt

    if (strcmp(test, "len=") != 0 || val == 0 ){

        free(lines);
        free(fileLenght);
        // Close and exit on error
    }

    /*
     * Write status=..., file=..., len=..., into to file ???
     */

    // int rounds_in_while = ((int)val/BUF_SIZE) + 1; // calculate how much lines it needs to make in the while for reading the file

    FILE *outputfile = fopen(*fileName, "w");

    if (outputfile == NULL) {
        //fprintf(stderr, "failure in copying file message");
        fclose(outputfile);
        // Close and exit on error
    }

    int read_lenght = 0;
    int max_read = 0;
    int foo = 0;
    char buffer [BUF_SIZE];


    while (read_lenght < lenght_of_file){

        if ((lenght_of_file - read_lenght) > BUF_SIZE) {

            max_read = BUF_SIZE;

        } else {

            max_read = lenght_of_file - read_lenght;

        }

        foo = 0;
        foo = fread(buffer, sizeof(char), max_read, fp_for_read);
        
        if (foo < 10 && foo < max_read){

            fclose(outputfile);
            fprintf(stderr, "reached EOF unaccpectad");
            break;
            
        }
        
        read_lenght += foo;
        
        if ((int)fwrite(buffer, sizeof(char), foo, outputfile) != max_read){

            fclose(outputfile);
            fclose(fp_for_read);
            fprintf(stderr, "error while writing into outputfile");
            break;
        }
    }
    
    free (fileLenght);
    free (fileName);
    
}


int send_Message_To_Server(int socket_file_descriptor, const char *message, const char *user, const char *img_url){

    size_t length_of_message;

    fp_for_write = fdopen(socket_file_descriptor,"w");


    // Sending message without img-URL

    if (img_url == NULL){

        if (feedback) {
            printf("function: send_Message_To_Server | trying: sending message without img-url to server\n");
        }

        if (fprintf(fp_for_write, "user=%s\n%s", user, message) < 0) {

            fprintf(stderr, "failed to write into file\n");
            fclose(fp_for_write);
            return -1;
        }

        // FLUSH !!

        if ((length_of_message = strlen(fp_for_write)) == 0) {

            fprintf(stderr, "failed calculation lenght of message \n");
            fclose(fp_for_write);
            return -1;
        }



        if (feedback) {
            printf("function: send_Message_To_Server | seccussful sending message without img-url to server\n");
        }

        // Sending message with img-URL

    } else {

        if (feedback) {
            printf("function: send_Message_To_Server | trying: sending message with img-url to server\n");
        }

        if (fprintf(fp_for_write, "user=%s\nimg=%s\n%s", user, img_url, message) < 0) {

            fprintf(stderr, "failed to write into file\n");
            fclose(fp_for_write);
            return -1;
        }

        // FLUSH!!

        if ((length_of_message = strlen(fp_for_write)) == 0) {

            fprintf(stderr, "failed calculation lenght of message \n");
            fclose(fp_for_write);
            return -1;
        }


        if (feedback) {
            printf("function: send_Message_To_Server | seccussful sending message with img-url to server\n");
        }

    }


}

int connect_With_Server(const char *server_address, const char *server_port) {

    struct addrinfo hints;
    struct addrinfo *serverinfo;
    struct addrinfo *rp;
    int return_value;
    int socket_file_descriptor;

    memset(&hints, 0, sizeof(hints));          /* Fill all the memory of hints with zeros */
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
        printf("function: connect_With_Server | successful connected to Server\n");
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

static void exit_on_error (int error, char* message) {

    if (error != 0) {
        fprintf(stderr, "%s: %s: %s\n", prog_name, message, strerror(error));
    }
    else {
        fprintf(stderr, "%s: %s\n", prog_name, message);
    }
    exit(EXIT_FAILURE);
}
