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
#include <fcntl.h>

void doStuff(int acceptfd, struct sockaddr_in cliaddr, pid_t pid);
void execute(char * command, char * ipaddress, char * hostname);
char* getDestination(char * command);
static char * readline(int s);
void log(const char *logentry);
void logtime();
void error_exit(const char *msg);

int dest, numRequests, numSecs, numUsers, port, minport, maxport;
int curUsers = 0;

// main server functionality
int main(int argc, char** argv) {
	// getting input arguments through command line
	char *portNumberStr;
	char *numRequestsStr;
	char *numSecsStr;
	char *numUsersStr;
	char *destStr;

	// getting input argumets through command line
	if (argc != 10) {
		portNumberStr = "1216";
		numRequestsStr = "2";
		numSecsStr = "6";
		numUsersStr = "2";
		destStr = "0";
	} else {
		portNumberStr = argv[2];
		numRequestsStr = argv[4];
		numSecsStr = argv[5];
		numUsersStr = argv[7];
		destStr = argv[9];
	}

	dest = atoi(destStr);
	numRequests = atoi(numRequestsStr);
	numSecs = atoi(numSecsStr);
	numUsers = atoi(numUsersStr);
	port = atoi(portNumberStr);
	minport = atoi("1025");
	maxport = atoi("65536");

	if (port < minport || port > maxport) {
		printf("Invalid port number. Port Number should be between 1025 and 65536");
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
    pid_t pid;
    
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

    // avoids hogging the port
    int n = 1;
    setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&n, sizeof (n));
    fcntl (sockfd, F_SETFD, 1);

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

    pid_t w;
    // int status;
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
        // child process
        if (pid == 0) {
            curUsers += 1;
            char output[50];
            sprintf(output, "User number: %d", curUsers);
            log(output);  
            sprintf(output, "pid: %d", pid);
            log(output);
            doStuff(acceptfd, cliaddr, pid);
        } else {
            // parent process
            int status;
            curUsers += 1;
            if (curUsers > numUsers) {
                wait(&status);
                curUsers--;
            }
        }
    }

	close(sockfd);
	return 0;
}

void doStuff(int acceptfd, struct sockaddr_in cliaddr, pid_t pid) {
	// reading data from client
    char * command;
    char * ipaddress = inet_ntoa(cliaddr.sin_addr);
    float allowance = (float) numRequests;
    time_t current;
    time_t lastCheck = time(NULL);
    long passed;
    
    /* Now connect standard output and standard error to the socket, instead of the invoking user’s terminal. */
    if (dup2 (acceptfd, 1) < 0 || dup2 (acceptfd, 2) < 0) {
        perror ("dup2");
        exit (1);
    }
    
    // kills process if max number of users reached
    if (curUsers > numUsers) {
        system("echo \"Max number of users reached, please try again later\"");
        // curUsers -= 1;
        // close(acceptfd);
        shutdown(acceptfd, 2);
        _exit(EXIT_FAILURE);
    }                    
    
    for (;;) {
        char output[50];
        sprintf(output, "Idle time: %ld", time(NULL));
        log(output);

        // reads data from client
	    command = readline(acceptfd);

        if (command == NULL) {
            logtime();
            log("error occurred while reading data from client");
            error_exit("error occurred while reading data from client");
        } else if (strcmp(command, "quit") == 0){
            curUsers--;
            shutdown(acceptfd, 2);
            // close(acceptfd);
            _exit(EXIT_SUCCESS);
            // kill(pid, SIGTERM);
            return;
        } else if (strcmp(command, "help") == 0) {
            log("client help command issued");
            system("echo \"server start up: <port number> <number requests> <number seconds> <number of users> <0 or1>\"");
            system("echo \"traceroute <hostname/ipaddress> - prints the trace of the route from server to host/ipaddress\"");
            system("echo \"traceroute me - prints the trace of the route from server to client\"");
            system("echo \"quit - close the connection and exit\"");
            continue;
        } else if (strcmp(command, "traceroute me") == 0) {
            char syscommand[1000];
            strcpy(syscommand, "traceroute ");
            strcat(syscommand, ipaddress);
            command = syscommand;
        }
        
        // calculates rate limiting
        current = time(NULL);
        passed = (long)(current - lastCheck);
        lastCheck = current;
        allowance += (float) passed * ((float) numRequests / numSecs);

        if (allowance > numRequests) {
           allowance = numRequests; // throttle
        } 
        
        if (allowance < 1.0) {
            system("echo \"Max number of requests reached, please try again later\"");            
        } else {
            allowance -= 1.0;
           
            struct hostent *he;
            he = gethostbyaddr((char *) &cliaddr.sin_addr, sizeof(cliaddr.sin_addr), AF_INET);
            char * hostname = he->h_name;
    
            // open file
            char *destination = getDestination(command);
            FILE *fr;
            fr = fopen(destination, "r");

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
}

char* getDestination(char * command) {
    int comlen = sizeof(command);
    char *destination;
    // strncpy(destination, command + 11, (comlen -11)); //TODO: get destination 
    return destination;
}

void execute(char * command, char * ipaddress, char * hostname) {

	char logentry[1000];
	strcpy(logentry, "traceroute issued to  ");
	strcat(logentry, command);
	log(logentry);
	strcpy(logentry, "client ip address  ");
	strcat(logentry, ipaddress);
	log(logentry);

	if (dest == 1) {
        char *destination = getDestination(command);
		int i = strcmp(ipaddress, destination);
		int j = strcmp(hostname, destination);
		if (i != 0 && j != 0) {
			logtime();
			log("users are only allowed to send traceroutes to their own IP addresses. request discarded!");
			system("echo \"users are only allowed to send traceroutes to their own IP addresses\"");
			return;
		}
	}

	int ret = system(command);
	strcpy(logentry, "trace route information sent to client");
	log(logentry);
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
	struct tm *myTime;
	char chrDate[20];
	time_t mytm;

	time(&mytm);
	myTime = localtime(&mytm);
	strftime(chrDate, 20, "%m/%d/%Y %H:%M:%S", myTime);
	log(chrDate);
}

// prints the error and exits
void error_exit(const char *msg) {
	perror(msg);
	exit(1);
}

/* Read a line of input from a file descriptor and return it. */
static char * readline(int s) {
	char *buf = NULL, *nbuf;
	int buf_pos = 0, buf_len = 0;
	int i, n;
	for (;;) {
		/* Ensure there is room in the buffer */
		if (buf_pos == buf_len) {
			buf_len = buf_len ? buf_len << 1 : 4;
			nbuf = realloc(buf, buf_len);
			if (!nbuf) {
				free(buf);
				return NULL ;
			}
			buf = nbuf;
		}

		/* Read some data into the buffer */
		n = read(s, buf + buf_pos, buf_len - buf_pos);
		if (n <= 0) {
			if (n < 0)
				perror("read");
			else
				fprintf(stderr, "read: EOF\n");
			free(buf);
			return NULL ;
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

