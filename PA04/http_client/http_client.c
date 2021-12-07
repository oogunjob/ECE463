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

char* getFileName(char* filepath);
int HTTPStatus(int sock);
int computeFileSize(int sock);

char* getFileName(char* filepath){
    char *filename = filepath + strlen(filepath); // filename
    
    // extracts filename from path
    for (; filename > filepath; filename--){
        if ((*filename == '\\') || (*filename == '/')){
            filename++;
            break;
        }
    }

    return filename; // returns filename
}

int computeFileSize(int sock){
    
    char buffer[1024];
    char* ptr = buffer + 4;
    
    int bytesReceived;
    
    while(bytesReceived = recv(sock, ptr, 1, 0)){
        if(bytesReceived < 0){
            perror("computeFileSize");
            exit(1);
        }

        if((ptr[-3] == '\r')  && (ptr[-2] == '\n' ) && (ptr[-1] == '\r')  && (*ptr == '\n')) 
            break;
        
        ptr++;
    }

    *ptr = 0;
    ptr = buffer + 4;

    if(bytesReceived){
        ptr = strstr(ptr,"fileSize");
        
        if(ptr){
            sscanf(ptr,"%*s %d",&bytesReceived);
        }
        else{
            bytesReceived = -1;
        }
    }
    
    return bytesReceived; // returns the number of bytes received
}

int HTTPStatus(int sock){

    char buffer[1024];
    char* ptr = buffer + 1;
    
    int bytesReceived;
    int status;
    
    while(bytesReceived = recv(sock, ptr, 1, 0)){
        if(bytesReceived < 0){
            perror("HTTPStatus");
            exit(1);
        }

        if((ptr[-1] == '\r')  && (*ptr == '\n')) 
            break;
        
        ptr++;
    }

    *ptr = 0;
    ptr = buffer + 1;

    sscanf(ptr,"%*s %d ", &status);

    if(bytesReceived > 0){
        return status; // returns file status size
    }

    return 0;
}

int main(int argc, char *argv[]){
    if (argc != 4) {
        fprintf(stderr,"usage: ./http_client [host] [port number] [filepath]\n");
        exit(1);
    }

    // stores command line arguments in variables
    char* domain = argv[1];
    int port = (int) strtol(argv[2], NULL, 10);
    char* filepath = argv[3];
    char* filename = getFileName(filepath);
    FILE* file = NULL;

    // variables for sock construction
    int sock;
    struct sockaddr_in server_addr;
    struct hostent *host;

    int bytesReceived;
    char send_data[1024];
    char recv_data[1024];

	if ((host = gethostbyname(domain)) == NULL) {
		herror("gethostbyname");
		exit(1);
	}

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr_list[0]);
	bzero(&(server_addr.sin_zero), 8);
	
    if (connect(sock, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) < 0) {
		perror("connect");
		exit(1);
	}

    // data to send to host
    snprintf(send_data, sizeof(send_data), "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", filepath, domain);

    if(send(sock, send_data, strlen(send_data), 0) < 0){
        perror("send");
        exit(1); 
    }
    
    int fileSize; // size of the file being downloaded

    if(HTTPStatus(sock) && (fileSize = computeFileSize(sock))){

        int bytes = 0; // current number of bytes written to file

        file = fopen(filename, "w"); // opens the file to be downloaded

        while(bytesReceived = recv(sock, recv_data, 1024, 0)){
            if(bytesReceived < 0){
                perror("recieve");
                exit(1);
            }

            // writes bytes to file
            fwrite(recv_data, 1, bytesReceived, file);
            bytes += bytesReceived;

            // stops receiving bytes when the number of current bytes reaches file size
            if(bytes == fileSize){
                break;
            }
        }

        fclose(file); // closes the file
    }

    close(sock); // closes the socket

    return 0;
}
