/* The code is subject to Purdue University copyright policies.
 * DO NOT SHARE, DISTRIBUTE, OR POST ONLINE
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MYPORT 8080 /* server should bind to port 8080 */
                    /* bind to IP address INADDR_ANY */

#define MAXPENDING 50 /* Maximum outstanding connection requests; listen() param */

#define DBADDR "127.0.0.1"
#define DBPORT 53004

#define MAXLINE 4096 /* Maximum chunk size */

int main(int argc , char *argv[])
{
	int sockfd , client_sock , c , read_size;
	struct sockaddr_in server , client;
	char client_message[2000];
	uint8_t buff[MAXLINE + 1];
	
	// creation of the socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	
	// creation of the the sockaddr_in structure for server
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(MYPORT);
  bzero(&(server.sin_zero), 8);
	
	// bind
	if (bind(sockfd, (struct sockaddr *) &server, sizeof(struct sockaddr)) < 0) {
		perror("bind");
		exit(1);
	}
	
  // listen for incoming connections
	if (listen(sockfd, MAXPENDING) < 0) {
		perror("listen");
		exit(1);
	}
	
  // while(1){
  //   socklen_t addrlen = sizeof(client);
  //   client_sock = accept(sockfd, (struct sockaddr *)&client, &addrlen);

  // }









	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	
	//accept connection from an incoming client
	client_sock = accept(sockfd, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_sock < 0)
	{
		perror("accept failed");
		return 1;
	}
	puts("Connection accepted");
	
	//Receive a message from client
	while( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0 )
	{
		//Send the message back to client
		fprintf(stdout, "%s\n", client_message);

		snprintf((char*)buff, sizeof(buff), "HTTP/1.0 200 OK\r\n\r\n<!doctype html><html>Hello World! </html>");
		int test = write(client_sock, (char*)buff, strlen((char*)buff));
	}
	
	if(read_size == 0)
	{
		puts("Client disconnected");
		fflush(stdout);
	}
	else if(read_size == -1)
	{
		perror("recv failed");
	}
	
	return 0;
}
