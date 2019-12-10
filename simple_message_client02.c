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

const char *prog_name = "";

static int verbose;

/*
 * -------------------------------------------------------- PROTOTYPES -------------------------------------------------
 */

void parse_Command(int argc, const char *argv[], const char **server, const char **port, const char **user, const char **message, const char **img_url, int *verbose);
void print_Manuel (FILE *fp , const char *program_name , int exit_value);
int connect_With_Server(const char *server_address, const char *server_port);
int send_Message_To_Server(int socket_file_descriptor, const char *message, const char *user, const char *img_url);
void receive_Message_From_Server(int socket_file_descriptor);
static void exit_on_error (int error, char* message);
size_t read_File (FILE *fp,char *filename, size_t lenght);


/*
 * -------------------------------------------------------- MAIN -------------------------------------------------------
 */

int main (int argc, const char *argv []) {

    const char *server = NULL;
    const char *port = NULL;
    const char *user = NULL;
    const char *message = NULL;
    const char *img_url = NULL;

    int socket_w = 0;
    int socket_r = 0;



    if (feedback){
        fprintf(stdout,"parsing the command\n");
    }

    parse_Command(argc,argv,&server, &port, &user, &message, &img_url, (int*)&verbose);

    if (argc < 2) {
        fprintf(stderr, "Not enough arguments\n");
    }

    if (feedback){
        fprintf(stdout,"connecting with server\n");
    }

    socket_w = connect_With_Server(server, port);

    socket_r = dup(socket_w);


    if (feedback){
        fprintf(stdout,"Sending message to server\n");
    }

    send_Message_To_Server(socket_w, message, user, img_url);

    if (feedback){
        fprintf(stdout,"Recieving message from server\n");
    }

    receive_Message_From_Server(socket_r);

    if (feedback){
        fprintf(stdout,"End programm\n");
    }

    /*
    shutdown(fp_for_read, SHUT_RD);
    shutdown(fp_for_write, SHUT_WR);

    close(fp_for_write);
    close(fp_for_read);

*/
    return EXIT_SUCCESS;
}

/*
 * -------------------------------------------------------- FUNCTIONS --------------------------------------------------
 */

void receive_Message_From_Server (int socket_file_descriptor) {

    char *filename;
    char buffer [BUF_SIZE];
    char *b = buffer;
    char *cutter;
    size_t lenght;
    size_t buffer_size = BUF_SIZE;
    size_t rt_value;

    //buffer (char*) malloc(BUF_SIZE);

    FILE *fp_for_read = fdopen(socket_file_descriptor, "r");

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | trying opening reading descriptor\n");
    }

    if (fp_for_read == NULL){

        fprintf(stderr, "Can not open file descriptor for reading");
        // error
    }

    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | opening reading descriptor successful\n");
    }


    if (feedback){
        fprintf(stdout,"function receive_Message_From_Server | trying reading status\n");
    }

    if (strcmp(buffer, "status=0\n") != 0){

        fprintf(stderr, "ERROR: status !=0\n");
        // Error
    }

    fprintf(stdout, "BUFFER:'%s'\n",buffer);


    while ((rt_value = getline(&b, &buffer_size, fp_for_read)) != 1) {

        if (ferror(fp_for_read)){
            fprintf(stderr, "Reached foep");
            // Error handler

        }


        if (feedback){
            fprintf(stdout,"function receive_Message_From_Server |trying reading file=\n");
        }

        if ((rt_value = getline(&b,&buffer_size,fp_for_read)) == -1) {

            fprintf(stderr, "Error while getting line for file=");
            // Error

        }

        if (strncmp(buffer, "file=", 5) != 0){

            fprintf(stderr, "Cant find file= in first line");
            // Error
        }

        fprintf(stdout, "BUFFER:'%s'\n",buffer);

        if (feedback){
            fprintf(stdout,"function receive_Message_From_Server | reading file= successful\ntrying storing filename\n");
        }

        filename = strdup(buffer+5);

        if (filename == NULL) {

            fprintf(stderr, "Error while writing the filename");

        }

        if (feedback){
            fprintf(stdout,"function receive_Message_From_Server | storing filename successful\n trying cutting of new line from filename\n");
        }


        cutter = strstr(filename,"\n");

        if (cutter == NULL){

            fprintf(stderr, "Error while cutting filename");

        }

        if (feedback){
            fprintf(stdout,"function receive_Message_From_Server | cutting of newline successful\ntrying getting row with len=\n");
        }

        cutter = '\0';

        buffer_size = rt_value +1;

        if ((rt_value = getline(&b,&buffer_size,fp_for_read)) == -1) {

            fprintf(stderr, "Error while getting line for filesize");
            // Error

        }

        if (feedback){
            fprintf(stdout,"function receive_Message_From_Server | reading line successful\ntrying getting the value of len=\n");
        }


        if (sscanf(buffer, "len=%lu", &lenght) < 1) {

            fprintf(stderr, "error while scan for lenght of file");

        }

        if (feedback){
            fprintf(stdout,"function receive_Message_From_Server | getting value of len successful\ntrying running function read_File\n");
        }

        fprintf(stdout, "BUFFER:'%s'\n",buffer);

        if (read_File(fp_for_read,filename,lenght) < lenght) {

            fprintf(stderr, "Reached EOF unexpacted");
            // Error

        }
    }
}


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
            fprintf(stdout,"Succesful writeing file");
        }
        return (already_write);
    }
}

int send_Message_To_Server(int socket_file_descriptor, const char *message, const char *user, const char *img_url){

    size_t length_of_message;

    FILE *fp_for_write = fdopen(socket_file_descriptor,"w");


    // Sending message without img-URL

    if (img_url == NULL){

        if (feedback) {
            fprintf(stdout,"function: send_Message_To_Server | trying: sending message without img-url to server\n");
        }

        if (fprintf(fp_for_write, "user=%s\n%s", user, message) < 0) {

            fprintf(stderr, "failed to write into file\n");
            fclose(fp_for_write);
            return -1;
        }

        if (feedback) {
            fprintf(stdout, "function: send_Message_To_Server | successful sending message without img-url to server | trying fflush\n");
        }

        if (fflush(fp_for_write )!= 0){

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
        }

        if (feedback) {
            fprintf(stdout,"function: send_Message_To_Server | successful shutdown\n");
        }

        fclose(fp_for_write);

        /*
        if ((length_of_message = strlen(fp_for_write)) == 0) {

            fprintf(stderr, "failed calculation lenght of message \n");
            fclose(fp_for_write);
            return -1;
        }

         */


        if (feedback) {
            fprintf(stdout,"function: send_Message_To_Server | successful sending message without img-url to server\n");
        }

        // Sending message with img-URL

    } else {

        if (feedback) {
            fprintf(stdout,"function: send_Message_To_Server | trying: sending message with img-url to server\n");
        }


        if (fprintf(fp_for_write, "user=%s\nimg=%s\n%s", user, img_url, message) < 0) {

            fprintf(stderr, "failed to write into file\n");
            fclose(fp_for_write);
            return -1;
        }

        if (feedback) {
            fprintf(stdout,"function: send_Message_To_Server | successful sending message with img-url to server | trying fflush\n");
        }

        if (fflush(fp_for_write) != 0){

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
        }

        if (feedback) {
            fprintf(stdout,"function: send_Message_To_Server | successful shutdown\n");
        }

        fclose(fp_for_write);

        // FLUSH!!

        /*
        if ((length_of_message = strlen(fp_for_write)) == 0) {

            fprintf(stderr, "failed calculation lenght of message \n");
            fclose(fp_for_write);
            return -1;
        }

         */

        if (feedback) {
            fprintf(stdout,"function: send_Message_To_Server | seccussful sending message with img-url to server\n");
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
        fprintf(stdout,"function: connect_With_Server | trying: running getaddrinfo\n");
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
        fprintf(stdout,"function: connect_With_Server | getaddrinfo successful\nfunction: connect_With_Server | trying: looping throw all sockets\n");
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
        fprintf(stdout,"function: connect_With_Server | successful connected to Server\n");
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
