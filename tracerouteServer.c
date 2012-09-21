#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

// prints the error and exits
void error_exit(const char *msg) {
	perror(msg);
	exit(1);
}

// write entries to log file
void log(const char *logentry) {
	FILE *file;
	file = fopen("log.txt", "a");
	if (!file) {
		printf("Error opening log file");
		exit(1);
	} else {
		fprintf(file, "%s\n", logentry);
		fclose(file);
	}
}

// write the current time to log file
void logtime() {
	char str[1000];
	char *logtime;
	time_t currenttime;
	currenttime = time(NULL );
	logtime = ctime(&currenttime);
	strcpy(str, logtime);
	log(str);
}

// main server functionality
int main(int argc, char** argv) {
	// getting input arguments through command line
	char *portNumber;
	char *numRequests;
	char *numSecs;
	char *numUsers;
	char *dest;

	// getting input argumets through command line
	if (argc != 6) {
		//printf("Usage:\n %s <port number> <number requests> <number seconds> <number of users> <0 or1>\n",argv[0]);
		portNumber = "1225";
		numRequests = "50";
		numSecs = "100";
		numUsers = "10";
		dest = "1";
	} else {

		portNumber = argv[1];
		numRequests = argv[2];
		numSecs = argv[3];
		numUsers = argv[4];
		dest = argv[5];
	}

	int port = atoi(portNumber);
	int minport = atoi("1025");
	int maxport = atoi("65536");

	if (port < minport || port > maxport) {
		printf(
				"Invalid port number. Port Number should be between 1025 and 65536");
	}
	logtime();

	// log input values
	char str[1000];
	strcpy(str, "server connecting on port : ");
	strcat(str, portNumber);
	strcat(str, " with maximum request count: ");
	strcat(str, numRequests);
	strcat(str, " in ");
	strcat(str, numSecs);
	strcat(str, " seconds.");
	log(str);
	strcpy(str, "Allowed maximum number of concurrent users are : ");
	strcat(str, numUsers);
	log(str);
	strcpy(str, "Strict destination enabled : ");
	strcat(str, dest);
	log(str);

	//returned values by the socket system call and the accept system call.
	int sockfd, acceptfd;
	// client address length, read write return value
	int cliaddrlen, rwretval;

	char readbuffer[256];

	struct sockaddr_in servaddr, cliaddr;

	// opening the socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		logtime();
		log("error occurred while opening the socket");
		error_exit("error occurred while opening the socket");
	}

	// initialize the server address to zero. set all the values in the buffer to 0
	bzero((char *) &servaddr, sizeof(servaddr));
	// populating server address
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(port);

	//binding the socket to port and host and listening
	if (bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		logtime();
		log("error occurred while binding the socket to server address");
		error_exit("error occurred while binding the socket to server address");
	}
	listen(sockfd, 5);

	// accepting client requests
	cliaddrlen = sizeof(cliaddr);
	acceptfd = accept(sockfd, (struct sockaddr *) &cliaddr, &cliaddr);
	if (acceptfd < 0) {
		logtime();
		log("error occurred while accepting client requests");
		error_exit("error occurred while accepting client requests");
	}

	// reading data from client
	bzero(readbuffer, 256);
	rwretval = read(acceptfd, readbuffer, 255);

	if (rwretval < 0) {
		logtime();
		log("error occurred while reading data from client");
		error_exit("error occurred while reading data from client");
	}
	printf("client says: %s\n", readbuffer);

	// validate the input

	// trace route functionality
	FILE *tracefp;
	char retrace[250];
	char trcommand[200];

	// check if the client sent a file name instead of IP address or a a host name
	FILE *fr;
	int n;
	char line[80];

	// open file
	fr = fopen("traceroutecommand.txt", "rwb");
	if (!fr) {

		char * ipaddress = inet_ntoa(cliaddr.sin_addr);
		printf("client ip address %s", ipaddress);
		printf("client input %s", readbuffer);

		// making sure that traceroute commands are executed against the client machines
		// there is an error with this method
		// have to compare the host name too
		// i was not able to get the clients host name
		/*
		 int deste = atoi(dest);
		 if (deste == 1) {
		 int i = strcmp(*ipaddress, *readbuffer);
		 printf("i %d", i);
		 if (i != 0) {
		 logtime();
		 log("users are only allowed to send traceroutes to their own IP addresses.");
		 rwretval = write(acceptfd, "users are only allowed to send traceroutes to their own IP addresses", 68);

		 }
		 }
		 */

		// Executes the traceroute command a system program
		rwretval = write(acceptfd, "I got your message", 18);

		/*
		 strcpy(trcommand, "traceroute google.com");
		 //strcat(trcoomand, readbuffer);
		 tracefp = popen(trcommand, "r");
		 while (fgets(retrace, sizeof(retrace) - 1, tracefp) != NULL ) {
		 printf("%s", retrace);
		 rwretval = write(acceptfd, retrace, 250);
		 if (rwretval < 0) {
		 logtime();
		 log("error occurred while writing the data to client");
		 error_exit("error occurred while writing the data to client");
		 }
		 }



		 //pclose(tracefp);
		 */

	} else {
		while (fgets(line, 80, fr) != NULL ) {
			sscanf(line, "%ld");
			tracefp = popen(line, "r");
			while (fgets(retrace, sizeof(retrace) - 1, tracefp) != NULL ) {
				printf("%s", retrace);
				rwretval = write(acceptfd, retrace, 250);
				if (rwretval < 0) {
					logtime();
					log("error occurred while writing the data to client");
					error_exit(
							"error occurred while writing the data to client");
				}
			}
			pclose(tracefp);
			rwretval = write(acceptfd, "============================================", 30);
		}
		fclose(fr);
	}
	close(acceptfd);
	close(sockfd);
	return 0;
}
