#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// prints the error and exits
void client_error_exit(const char *msg) {
	perror(msg);
	exit(1);
}

// client to connect & traceroute
int main(int argc, char *argv[]) {
	//returned values by the socket system call.
	int sockfd;
	int port;

	// read write return value
	int n;
	struct sockaddr_in servaddr;
	//pointer to a structure of type hostent which deifne the host
	struct hostent *server;

	char buffer[1024];
	if (argc < 3) {
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(0);
	}
	port = atoi(argv[2]);

	// opening the socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		client_error_exit("error while opening the socket");
	server = gethostbyname(argv[1]);
	if (server == NULL ) {
		printf("Invalid host name.");
		return 0;
	}

	// initialize the server address to zero. set all the values in the buffer to 0
	bzero((char *) &servaddr, sizeof(servaddr));

	// populating server address
	servaddr.sin_family = AF_INET;
	bcopy((char *) server->h_addr,
	(char *)&servaddr.sin_addr.s_addr,
	server->h_length);
	servaddr.sin_port = htons(port);

	//Establishing connection with the server
	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
		client_error_exit("error occurred while connecting with the server");

	while (1 == 1) {
		printf("Please enter the command: ");
		bzero(buffer, 1024);

		// gets the command from the user
		fgets(buffer, 1023, stdin);

		// sends the command to server
		n = write(sockfd, buffer, strlen(buffer));
		if (n < 0)
			client_error_exit(
					"error occurred while writing the data to socket");
		if (strstr(buffer, "quit") != NULL ) {
			break;
		} else {
			while (n > 0) {
				bzero(buffer, 1024);
				// read the server response
				n = read(sockfd, buffer, 1023);
				if (n < 0)
					client_error_exit(
							"error occurred while reading from socket");


				//checking if all the traces are read
				if (strstr(buffer, "##END##") != NULL ) {
					break;
				}
				printf("%s\n", buffer);
			}
		}
	}
	close(sockfd);
	return 0;
}

