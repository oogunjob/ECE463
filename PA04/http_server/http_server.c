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

#include <ctype.h>

#define MYPORT 8080 /* server should bind to port 8080 */
                    /* bind to IP address INADDR_ANY */

#define MAXPENDING 50 /* Maximum outstanding connection requests; listen() param */

#define DBADDR "127.0.0.1"
#define DBPORT 53004

#define MAXLINE 4096 /* Maximum chunk size */
#define CLIENT_MAXLINE 99999 /* Maximum message size */

void respond(int client_sock, struct sockaddr_in client, int database_sock, struct sockaddr_in database);

int main(){
	int sockfd , client_sock, database_sock;
	struct sockaddr_in server , client;
	
	// creation of the TCP socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	// creation of the the sockaddr_in structure for server
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(MYPORT);
  bzero(&(server.sin_zero), 8);
	
	// binds the connection from client to server
	if (bind(sockfd, (struct sockaddr *) &server, sizeof(struct sockaddr)) < 0) {
		perror("bind");
		exit(1);
	}
	
  // listen for incoming connections
	if (listen(sockfd, MAXPENDING) < 0) {
		perror("listen");
		exit(1);
	}

  // creation of UDP socket
  struct sockaddr_in database;
	
  if ((database_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

  struct timeval timeout;      
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;

  if (setsockopt(database_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0){
    perror("setsockopt failed");
  }

  if (setsockopt(database_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout) < 0){
    perror("setsockopt failed");
  }

  struct hostent *he;
  if ((he = gethostbyname(DBADDR)) == NULL) {
		perror("gethostbyname");
		exit(1);
	}
  
  database.sin_family = AF_INET;
  database.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
  database.sin_port = htons(DBPORT);

  // accepts all incoming connections from the client
  while(1){
    socklen_t addrlen = sizeof(client);
    client_sock = accept(sockfd, (struct sockaddr *)&client, &addrlen);
    
    if(client_sock < 0){
      perror("accept");
      exit(1);
    }

    else{
      if(fork() == 0){
        respond(client_sock, client, database_sock, database);
      }
    }
  }
	
	return 0;
}

void respond(int client_sock, struct sockaddr_in client, int database_sock, struct sockaddr_in database){
  char* WEBROOT = "Webpage"; // web root directory

  char dst[INET_ADDRSTRLEN];
	char clientMessage[CLIENT_MAXLINE], *requestLine[3], data_to_send[MAXLINE], path[MAXLINE];
  int file;
	int rcvd, bytes_read;
  int ret; // return value

	memset((void*)clientMessage, (int)'\0', CLIENT_MAXLINE);
	rcvd = recv(client_sock, clientMessage, CLIENT_MAXLINE, 0);

  // error receiving message
  if(rcvd < 0){
    perror("rcvd");
    exit(1);
  }
	
  // message was properly received and logs the client request to terminal
  else{
    // retrieves and prints client IP address
    inet_ntop(AF_INET, &(client.sin_addr), dst, INET_ADDRSTRLEN);
    fprintf(stdout, "%s ", dst);
    
    // prints the client's message to terminal
    int i = 0;
    fprintf(stdout, "\"");
    while(isprint(clientMessage[i])){
		  fprintf(stdout, "%c", clientMessage[i++]);
	  }
    fprintf(stdout, "\" ");
    
    requestLine[0] = strtok(clientMessage, " \t\n"); // method that was used in request (GET, POST, HEAD, etc.)

    // if the first line of the request was a GET method, look for the file requested in web root
		if(strncmp(requestLine[0], "GET\0", 4) == 0){ 
			
      requestLine[1] = strtok(NULL, " \t"); // file requested
			requestLine[2] = strtok(NULL, " \t\n"); // HTTP version

      // checks if the GET request has no '/' but still contains text (ex: "GET cutecat.jpg HTTP/1.0") or tries to access private content ("../" or "/..")
      if(strncmp(requestLine[1], "/", 1) != 0 || strstr(requestLine[1], "../") != NULL || strstr(requestLine[1], "/..") != NULL){
        fprintf(stdout, "400 Bad Request\n");
        send(client_sock, "HTTP/1.0 400 Bad Request\r\n", 24, 0);
        ret = write(client_sock, "HTTP/1.0 400 Bad Request\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>", 74);
      }

      // if the the protocol is neither HTTP/1.0 or HTTP/1.1, indicate that the request is bad, ask TA about this
			else if(strncmp(requestLine[2], "HTTP/1.0", 8) != 0 && strncmp(requestLine[2], "HTTP/1.1", 8) != 0){
          fprintf(stdout, "501 Not Implemented\n");
          send(client_sock, "HTTP/1.0 501 Not Implemented\r\n", 30, 0);
          ret = write(client_sock, "HTTP/1.0 501 Not Implemented\r\n\r\n<html><body><h1>501 Not Implemented</h1></body></html>", 86);
			}

			else{
        // if no specific file is specified, use index.html as the default page to display
				if(strncmp(requestLine[1], "/\0", 2) == 0)
					requestLine[1] = "/index.html";

        // appends the file path to to the web root (Webpage)
				strcpy(path, WEBROOT);
				strcpy(&path[strlen(WEBROOT)], requestLine[1]);

        // indication that the requested path was found in the web root
				if((file = open(path, O_RDONLY)) > 0){
          fprintf(stdout, "200 OK\n");
          struct stat *buf = malloc(sizeof(struct stat));
          stat(path, buf);
          int size = buf->st_size;
          free(buf);

          int sizeLen = snprintf( NULL, 0, "%d", size);
          char* sizeStr = malloc(sizeLen + 1);
          snprintf(sizeStr, sizeLen + 1, "%d", size);
          char final[sizeLen + 4 + 34];
          strcpy(final, "HTTP/1.0 200 OK\r\nContent-Length: ");
          strncat(final, sizeStr, sizeLen);
          strncat(final, "\r\n\r\n", 4);

					send(client_sock, final, strlen(final), 0);
          free(sizeStr);

          // writes the contents of the file back to the client
          while((bytes_read = read(file, data_to_send, MAXLINE)) > 0)
						ret = write(client_sock, data_to_send, bytes_read);
				}

        // checks if the file path is a request to search the data base
        else if(strstr(requestLine[1], "?key") != NULL){
          char* seperator = strtok(requestLine[1], "=");
          char* filename = strtok(NULL, ""); // gets the file name
          
          // replace all occurences of '+' with ' '
          for(int j = 0; j < strlen(filename); j++){
            if(filename[j] == '+')
              filename[j] = ' ';
          }

          // send's the request to the databsae
          if(sendto(database_sock, (const char*)filename, strlen(filename), 0, (const struct sockaddr*)&database, sizeof(database)) < 0){
            perror("sendto");
          }

          // checks for a 408 Request Timeout if it takes longer than 5 seconds to make a connection the database
          if(errno == EAGAIN || errno == EWOULDBLOCK){
              fprintf(stdout, "408 Request Timeout\n");
              send(client_sock, "HTTP/1.0 408 Request Timeout\r\n", 30, 0);
              ret = write(client_sock, "HTTP/1.0 408 Request Timeout\r\n\r\n<html><body><h1>408 Request Timeout</h1></body></html>", 86);
          }

          else{
            int len; // length of the database struct
            char buffer[MAXLINE]; // buffer to store the response from the database

            // receive database's response
            bytes_read = recvfrom(database_sock, (char*)buffer, MAXLINE, 0, (struct sockaddr*)&database, &len);

            // checks for a 408 Request Timeout if it takes longer than 5 seconds to make a connection the database
            if(errno == EAGAIN || errno == EWOULDBLOCK){
              fprintf(stdout, "408 Request Timeout\n");
              send(client_sock, "HTTP/1.0 408 Request Timeout\r\n", 30, 0);
              ret = write(client_sock, "HTTP/1.0 408 Request Timeout\r\n\r\n<html><body><h1>408 Request Timeout</h1></body></html>", 86);
            }

            // if the file was not found, return a 404 erorr code as indication
            else if (strstr(buffer, "File Not Found") != NULL){
              fprintf(stdout, "404 Not Found\n");
              send(client_sock, "HTTP/1.0 404 Not Found\r\n", 24, 0);
              ret = write(client_sock, "HTTP/1.0 404 Not Found\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>", 74);
            }

            // if the picture was successfully found, return a 200 success message and send the response back to the server
            else{
              send(client_sock, "HTTP/1.0 200 OK\r\nContent-Type: image/jpeg\r\n\r\n", 45, 0);
              ret = write(client_sock, buffer, bytes_read);

              while((bytes_read = recvfrom(database_sock, (char*)buffer, MAXLINE, 0, (struct sockaddr*)&database, &len)) > 0){
                if(strstr(buffer, "DONE") != NULL){
                  fprintf(stdout, "200 OK\n");
                  send(client_sock, "HTTP/1.0 200 OK\n\n", 17, 0);
                  break;
                }
                ret = write(client_sock, buffer, bytes_read);
              }
            }
          }
        }

        // indication that the requested path was NOT found in the web root nor data base
				else{
            fprintf(stdout, "404 Not Found\n");
            send(client_sock, "HTTP/1.0 404 Not Found\r\n", 24, 0);
            ret = write(client_sock, "HTTP/1.0 404 Not Found\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>", 74);
        }
			}
		}

    // indicates that the request was NOT a GET method (POST, HEAD, PUT)
    else{
        fprintf(stdout, "501 Not Implemented\n");
        send(client_sock, "HTTP/1.0 501 Not Implemented\r\n", 30, 0);
        ret = write(client_sock, "HTTP/1.0 501 Not Implemented\r\n\r\n<html><body><h1>501 Not Implemented</h1></body></html>", 86);
    }
	}

	// closes socket
	shutdown(client_sock, SHUT_RDWR);
	close(client_sock);
}