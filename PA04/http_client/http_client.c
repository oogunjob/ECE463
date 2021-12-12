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

    // first thing that needs to to be done is check the http status
    // IF THE RESPONSE IS NOT 200, PRINT IT AND EXIT THE PROGRAM'
    if(CheckStatus(sock) == 200){
        
        // if the response is 200 OK, check if the content size is correct and save the content to a filepath
        if(contentLength = CheckContentLength(sock)){
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

        // if the resonse did not contain the content length, print an error message
        else{
            fprintf(stdout, "Error: could not download the requested file (file length unknown)\n");
        }

    // prints the first line of the response from the server
    }
    else{
        fprintf(stdout, "The server responded with: \n");
    }
    
    close(sock); // closes the socket
    
    return 0;
}

char* getFileName(char* filepath){
    char *filename; // filename
    filename = filepath + strlen(filepath);
    
    // extracts filename from path
    for (; filename > filepath; filename--){
        if ((*filename == '\\') || (*filename == '/')){
            filename++;
            break;
        }
    }
    
    fprintf(stdout, "%s\n", filename);
    return filename; // returns filename
}

int CheckContentLength(int sock){
    char c;
    
    char buff[MAXBUFFER] = "";
    char* ptr= buff + 4;
    
    int bytes_received;
    int status;
    
    printf("Begin HEADER ..\n");
    while(bytes_received = recv(sock, ptr, 1, 0)){
        if(bytes_received==-1){
            perror("Parse Header");
            exit(1);
        }

        if((ptr[-3]=='\r')  && (ptr[-2]=='\n' ) && (ptr[-1]=='\r')  && (*ptr=='\n')) 
            break;
        
        ptr++;
    }

    *ptr = 0;
    ptr = buff + 4;
    //printf("%s",ptr);

    if(bytes_received){
        ptr = strstr(ptr,"Content-Length:");
        if(ptr){
            sscanf(ptr,"%*s %d",&bytes_received);

        }else
            bytes_received=-1; //unknown size

       printf("Content-Length: %d\n",bytes_received);
    }
    printf("End HEADER ..\n");
    
    return bytes_received;
}

int CheckStatus(int sock){

    char buff[MAXBUFFER] = "";
    char* ptr = buff + 1;
    int bytes_received, status;

    while(bytes_received = recv(sock, ptr, 1, 0)){
        if(bytes_received == -1){
            perror("ReadHttpStatus");
            exit(1);
        }

        if((ptr[-1] == '\r')  && (*ptr =='\n')) break;
        ptr++;
    }

    *ptr=0;
    ptr = buff + 1;

    sscanf(ptr,"%*s %d ", &status);

    printf("%s\n", ptr); // prints entire messsage, NEED TO REMOVE
    printf("status = %d\n",status); // prints what the status of the request was, NEED TO REMOVE

    return status; // returns the HTTP Status code
}