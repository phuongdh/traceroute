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

void doStuff(int acceptfd, struct sockaddr_in cliaddr);
void execute(char * command, char * ipaddress, char * hostname);
static char * readline (int s);
void log(const char *logentry);
void logtime();
void error_exit(const char *msg);

int dest, numRequests, numSecs, numUsers, port, minport, maxport;

// main server functionality
int main(int argc, char** argv) {
	// getting input arguments through command line
	char *portNumberStr;
	char *numRequestsStr;
	char *numSecsStr;
	char *numUsersStr;
	char *destStr;

	// getting input argumets through command line
	if (argc != 6) {
		//printf("Usage:\n %s <port number> <number requests> <number seconds> <number of users> <0 or1>\n",argv[0]);
		portNumberStr = "1225";
		numRequestsStr = "50";
		numSecsStr = "100";
		numUsersStr = "10";
		destStr = "0";
	} else {
		portNumberStr = argv[1];
		numRequestsStr = argv[2];
		numSecsStr = argv[3];
		numUsersStr = argv[4];
		destStr = argv[5];
	}

    dest = atoi(destStr);
    numRequests = atoi(numRequestsStr);
    numSecs = atoi(numSecsStr);
    numUsers = atoi(numUsersStr);
	port = atoi(portNumberStr);
	minport = atoi("1025");
	maxport = atoi("65536");

	if (port < minport || port > maxport) {
		printf(
				"Invalid port number. Port Number should be between 1025 and 65536");
	}
	logtime();

	// log input values
	char str[1000];
	strcpy(str, "server connecting on port : ");
	strcat(str, portNumberStr);
	strcat(str, " with maximum request count: ");
	strcat(str, numRequestsStr);
	strcat(str, " in ");
	strcat(str, numSecsStr);
	strcat(str, " seconds.");
	log(str);
	strcpy(str, "Allowed maximum number of concurrent users are : ");
	strcat(str, numUsersStr);
	log(str);
	strcpy(str, "Strict destination enabled : ");
	strcat(str, destStr);
	log(str);

	//returned values by the socket system call and the accept system call.
	int sockfd, acceptfd;
    int pid;
    
	// client address length, read write return value
	int cliaddrlen;
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

    for (;;) {
    	// accepting client requests
    	cliaddrlen = sizeof(cliaddr);
    	acceptfd = accept(sockfd, (struct sockaddr *) &cliaddr, &cliaddrlen);
    	if (acceptfd < 0) {
    		logtime();
    		log("error occurred while accepting client requests");
    		error_exit("error occurred while accepting client requests");
    	}

        pid = fork ();
        if (!pid) {
            doStuff(acceptfd, cliaddr);
        }
    }

	close(sockfd);
	return 0;
}

void doStuff(int acceptfd, struct sockaddr_in cliaddr) {
	// reading data from client
    char * command;
    // time_t currenttime;
    // currenttime = time(NULL );
    
	while (1==1) {
	    command = readline(acceptfd);

        if (command == NULL) {
            logtime();
            log("error occurred while reading data from client");
            error_exit("error occurred while reading data from client");
        } else if (strcmp(command, "quit") == 0){
            close(acceptfd);
            return;
        }

        /* Now connect standard output and standard error to the socket, instead of the invoking user’s terminal. */
        if (dup2 (acceptfd, 1) < 0 || dup2 (acceptfd, 2) < 0) {
            perror ("dup2");
            exit (1);
        }

        char * ipaddress = inet_ntoa(cliaddr.sin_addr);
        char * hostname = "myhostname"; //TODO: get the actual hostname
    
        // open file
        FILE *fr;
        fr = fopen(command, "r");

        if (fr) {
            // Reads and executes each line if file exists
            int n;
            char line[80];
            while (fgets(line, 80, fr) != NULL ) {
                execute(line, ipaddress, hostname);
                printf("============================================\n");
            }
            fclose(fr);
        } else {
            execute(command, ipaddress, hostname);
        }
    }
}

void execute(char * command, char * ipaddress, char * hostname) {
    log(ipaddress);
    if (dest == 1) {
        int i = strcmp(ipaddress, command);
        int j = strcmp(hostname, command);
        if (i != 0 && j != 0) {
            logtime();
            log("users are only allowed to send traceroutes to their own IP addresses.");
            printf("users are only allowed to send traceroutes to their own IP addresses");
            return;
        }
    }

    int ret = system(command);
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
    return;
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

// prints the error and exits
void error_exit(const char *msg) {
	perror(msg);
	exit(1);
}

/* Read a line of input from a file descriptor and return it. */
static char * readline (int s)
{
    char *buf = NULL, *nbuf;
    int buf_pos = 0, buf_len = 0;
    int i, n;
    for (;;) {
        /* Ensure there is room in the buffer */
        if (buf_pos == buf_len) {
            buf_len = buf_len ? buf_len << 1 : 4;
            nbuf = realloc (buf, buf_len);
            if (!nbuf) {
                free (buf);
                return NULL;
            }
            buf = nbuf;
        }

        /* Read some data into the buffer */
        n = read (s, buf + buf_pos, buf_len - buf_pos);
        if (n <= 0) {
            if (n < 0)
                perror ("read");
            else
                fprintf (stderr, "read: EOF\n");
            free (buf);
            return NULL;
        }

        /* Look for the end of a line, and return if we got it.*/
        for (i = buf_pos; i < buf_pos + n; i++)
            if (buf[i] == '\0' || buf[i] == '\r' || buf[i] == '\n') {
                buf[i] = '\0';
                return buf;
            }

        buf_pos += n;
    }
}
