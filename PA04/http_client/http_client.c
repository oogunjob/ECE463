/* The code is subject to Purdue University copyright policies.
 * DO NOT SHARE, DISTRIBUTE, OR POST ONLINE
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAXBUFFER 4096

char* getFileName(char* filepath);
int CheckStatus(int sock);
int CheckContentLength(int sock);

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr,"usage: ./http_client [host] [port number] [filepath]\n");
        exit(1);
    }

    // stores command line arguments in variables
    char* domain = argv[1];
    int PORT = (int) strtol(argv[2], NULL, 10);
    char* filepath = argv[3];
    char* filename = getFileName(filepath);

    // variables for sock construction
    int sock;
    struct sockaddr_in server_addr;
    struct hostent *host;

    int bytesReceived;
    char send_data[MAXBUFFER];
    char recv_data[MAXBUFFER];
    char clientMessage[MAXBUFFER];
    int contentLength;

    // connects to the host
	if ((host = gethostbyname(domain)) == NULL) {
		herror("gethostbyname");
		exit(1);
	}

    // creation of TCP socket
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

    // creates assignment of socket attributes
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr_list[0]);
	bzero(&(server_addr.sin_zero), 8);
	
    // connection to the socket
    if (connect(sock, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) < 0) {
		perror("connect");
		exit(1);
	}

    // GET request that is sent to the server
    snprintf(send_data, sizeof(send_data), "GET /%s HTTP/1.1\r\nHost: %s:%s\r\n\r\n", filepath, domain, argv[2]);

    // sends the request to the server
    if(send(sock, send_data, strlen(send_data), 0) < 0){
        perror("send");
        exit(1); 
    }

    // check if the response status was '200 OK'
    if(CheckStatus(sock) == 200){
        
        // if the response is 200 OK, check if the Content-Length was valid, if so write the file
        if(contentLength = CheckContentLength(sock) && contentLength != -1){
            // save to the file path
            
            int bytes = 0; // current number of bytes written to file
            FILE* file = fopen(filename, "w"); // opens the file to be written to

            while(bytesReceived = recv(sock, recv_data, MAXBUFFER, 0)){
                if(bytesReceived == -1){
                    perror("recieve");
                    exit(1);
                }

                fwrite(recv_data, 1, bytesReceived, file); // writes bytes to file
                bytes += bytesReceived;

                if(bytes == contentLength){
                    break;
                }
            }

            fclose(file); // closes the file on completion
        }

        // if the response did not contain the content length, print an error message
        else{
            fprintf(stdout, "Error: could not download the requested file (file length unknown)\n");
        }
    }
    
    close(sock); // closes the socket
    return 0;
}

char* getFileName(char* filepath){
    char *filename; // filename
    filename = filepath + strlen(filepath);
    
    // extracts filename from path
    for (; filename > filepath; filename--){
        if((*filename == '\\') || (*filename == '/')){
            filename++;
            break;
        }
    }

    //if the filename's first character is a '/', remove it
    if(filename[0] == '/'){
        filename++;
    }
    
    return filename; // returns filename
}

int CheckContentLength(int sock){
    char buff[MAXBUFFER] = "";
    char* ptr = buff + 4;
    
    int bytes_received;
    int contentLength;
    
    while(bytes_received = recv(sock, ptr, 1, 0)){
        if(bytes_received == -1){
            perror("CheckContentLength");
            exit(1);
        }

        if((ptr[-3] =='\r')  && (ptr[-2] =='\n' ) && (ptr[-1] == '\r')  && (*ptr == '\n')) 
            break;
        
        ptr++;
    }

    *ptr = 0;
    ptr = buff + 4;

    // checks if the content legnth was in the response header
    if(bytes_received){
        ptr = strstr(ptr,"Content-Length:");
        if(ptr){
            // stores the content length
            sscanf(ptr,"%*s %d", &contentLength);
        }

        // Content-Length was not present in the server response
        else{
            contentLength = -1;
        }
    }

    return contentLength;
}

int CheckStatus(int sock){

    char buff[MAXBUFFER] = "";
    char* ptr = buff + 1;
    int bytes_received, status;

    // retrieves the server response
    while(bytes_received = recv(sock, ptr, 1, 0)){
        if(bytes_received == -1){
            perror("CheckStatus");
            exit(1);
        }

        if((ptr[-1] == '\r')  && (*ptr =='\n')) break;
        ptr++;
    }

    *ptr = 0;
    ptr = buff + 1;

    // checks if a 200 OK response was received
    if(strstr(ptr, "200 OK")){
        status = 200;
    }

    // if 200 OK was not received, print the error message from the server
    else{
        fprintf(stdout, "%s\n", ptr);
    }

    return status; // returns the HTTP Status code
}