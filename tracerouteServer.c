/*
 * authors : Phuong Hoang Dinh, Bimalee Salpitikorala
 *
 * references
 * http://www.linuxhowtos.org/C_C++/socket.htm
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 */

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
#include <regex.h>

void doStuff(int acceptfd, struct sockaddr_in cliaddr, pid_t pid);
void execute(char * command, char * ipaddress, char * hostname);
char* getDestination(char * command);
void log(const char *logentry, char * ipaddress);
void logtime();
void error_exit(const char *msg);
int validatehostname(char * hostname);
int validateipaddress(char * ipaddress);
char * trim (char *s);

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

	portNumberStr = "1216";
	numRequestsStr = "4";
	numSecsStr = "60";
	numUsersStr = "2";
	destStr = "0";
	port = atoi(portNumberStr);
	minport = atoi("1025");
	maxport = atoi("65536");

	// getting input arguments through command line
	int i;
	if (argc > 1) {
		for (i = 0; i < argc; i++) {

			if (strcmp(argv[i], "PORT") == 0) {
				portNumberStr = argv[i + 1];
				port = atoi(portNumberStr);
				if (port < minport || port > maxport) {
					system(
							"echo \"Invalid port number. default port 1216 taken instead. Port Number should be between 1025 and 65536\"");
				}
			} else if (strcmp(argv[i], "RATE") == 0) {
				numRequestsStr = argv[i + 1];
				numSecsStr = argv[i + 2];
			} else if (strcmp(argv[i], "MAX_USERS") == 0) {
				numUsersStr = argv[i + 1];
			} else if (strcmp(argv[i], "STRICT_DEST") == 0) {
				destStr = argv[i + 1];
			}

		}
	}

	dest = atoi(destStr);
	numRequests = atoi(numRequestsStr);
	numSecs = atoi(numSecsStr);
	numUsers = atoi(numUsersStr);

	// log input values
	char str[1000];
	strcpy(str, "server connecting on port : ");
	strcat(str, portNumberStr);
	strcat(str, " with maximum request count: ");
	strcat(str, numRequestsStr);
	strcat(str, " in ");
	strcat(str, numSecsStr);
	strcat(str, " seconds.");
	log(str, NULL);
	strcpy(str, "Allowed maximum number of concurrent users are : ");
	strcat(str, numUsersStr);
	log(str, NULL);
	strcpy(str, "Strict destination enabled : ");
	strcat(str, destStr);
	log(str, NULL);

	//returned values by the socket system call and the accept system call.
	int sockfd, acceptfd;
	pid_t pid;

	// client address length, read write return value
	int cliaddrlen;
	struct sockaddr_in servaddr, cliaddr;

	// opening the socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		//logtime();
		log("error occurred while opening the socket", NULL);
		error_exit("error occurred while opening the socket");
	}

	// avoids hogging the port
	int n = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &n, sizeof(n));
	fcntl(sockfd, F_SETFD, 1);

	// initialize the server address to zero. set all the values in the buffer to 0
	bzero((char *) &servaddr, sizeof(servaddr));
	// populating server address
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(port);

	// binding the socket to port and host and listening
	if (bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		//logtime();
		log("error occurred while binding the socket to server address", NULL);
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
			log("error occurred while accepting client requests", NULL);
			error_exit("error occurred while accepting client requests");
		}

		curUsers += 1;
		int status;

		for (; waitpid(-1, &status, WNOHANG) > 0; --curUsers) {
		}

		for (; curUsers > numUsers + 1; --curUsers)
			wait(&status);

		pid = fork();

		if (pid == 0) {
			// runs child process
			doStuff(acceptfd, cliaddr, pid);
		}
	}

	close(sockfd);
	return 0;
}

void doStuff(int acceptfd, struct sockaddr_in cliaddr, pid_t pid) {
    char * readline(int s);
    fd_set fdset;

    struct timeval timeout;
    FD_ZERO(&fdset);
    FD_SET(acceptfd, &fdset);
    timeout.tv_sec = 30;          
    timeout.tv_usec = 0;
    
	// reading data from client
	char * command;
	char * ipaddress = inet_ntoa(cliaddr.sin_addr);
	float allowance = (float) numRequests;
	time_t current;
	time_t lastCheck = time(NULL );
	long passed;

	/* Now connect standard output and standard error to the socket, instead of the invoking user’s terminal. */
	if (dup2(acceptfd, 1) < 0 || dup2(acceptfd, 2) < 0) {
		perror("dup2");
		exit(1);
	}

	// kills process if max number of users reached
	if (curUsers > numUsers) {
		system("echo \"Max number of users reached, please try again later\"");
		log("client disconnected due to max number of users reached", ipaddress);
		shutdown(acceptfd, 2);
		_exit(EXIT_FAILURE);
	}

	for (;;) {
		while (1 == 1) {
			// reads data from client
            if (select(sizeof(fdset)*8, &fdset, NULL, NULL, &timeout) > 0)
            {
    			command = readline(acceptfd);

            } else
            {
				log("client timed out", ipaddress);
				system("echo \"no command issue for a long period. You have been disconnected!\"");
				system("echo \"##END##\"");
        		shutdown(acceptfd, 2);
        		_exit(EXIT_FAILURE);
            }

			if (command == NULL ) {
				logtime();
				log("error occurred while reading data from client", ipaddress);
				error_exit("error occurred while reading data from client");
			} else if (strcmp(command, "quit") == 0) {
				shutdown(acceptfd, 2);
				_exit(EXIT_SUCCESS);
				return;
			} else if (strcmp(command, "help") == 0) {
				log("client help command issued", ipaddress);
				system(
						"echo \"traceroute <hostname/ipaddress> - prints the trace of the route from server to host/ipaddress\"");
				system(
						"echo \"traceroute me - prints the trace of the route from server to client\"");
				system("echo \"quit - close the connection and exit\"");
				system("echo \"##END##\"");
				break;
			} else if (strcmp(command, "traceroute me") == 0) {
				char syscommand[1000];
				strcpy(syscommand, "traceroute ");
				strcat(syscommand, ipaddress);
				command = syscommand;

			} else if (strstr(command, "traceroute ") == NULL ) {
				system(
						"echo \"Invalid command. please type help to see the options\"");
				system("echo \"##END##\"");
				log(" Invalid command issued", ipaddress);
				break;
			}

				struct hostent *he;
				he = gethostbyaddr((char *) &cliaddr.sin_addr,
						sizeof(cliaddr.sin_addr), AF_INET);
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
            			// calculates rate limiting
            			current = time(NULL );
            			passed = (long) (current - lastCheck);
            			lastCheck = current;
            			allowance += (float) passed * ((float) numRequests / numSecs);

            			// resets rate
            			if (allowance > numRequests) {
            				allowance = numRequests;
            			}

            			if (allowance < 1.0) {
            				system(
            						"echo \"Max number of requests reached, please try again later\"");
            				system("echo \"##END##\"");
            				log("Max number of commands exceeds. command discarded ", ipaddress);
            			} else {
            				allowance -= 1.0;
    						execute(line, ipaddress, hostname);
    						system(
    								"echo \"============================================\n\"");
                        }
					}
					system("echo \"##END##\"");
					fclose(fr);
				} else {
        			// calculates rate limiting
        			current = time(NULL );
        			passed = (long) (current - lastCheck);
        			lastCheck = current;
        			allowance += (float) passed * ((float) numRequests / numSecs);

        			// resets rate
        			if (allowance > numRequests) {
        				allowance = numRequests;
        			}

        			if (allowance < 1.0) {
        				system(
        						"echo \"Max number of requests reached, please try again later\"");
        				system("echo \"##END##\"");
        				log("Max number of commands exceeds. command discarded ", ipaddress);
        			} else {
        				allowance -= 1.0;
                    
    					execute(command, ipaddress, hostname);
    					system("echo \"##END##\"");
                    }
				}
			
		}
	}
}

char* getDestination(char * command) {
	return strndup(command + 11, (sizeof(command) - 11));
}

void execute(char * command, char * ipaddress, char * hostname) {

	char logentry[1000];
	strcpy(logentry, "traceroute issued to  ");
	strcat(logentry, command);
	log(logentry, ipaddress);

	char *destination = getDestination(trim(command));
	int validh = validatehostname(destination);
	if (validh == -1) {
		int validip = validateipaddress(destination);
		if (validip == -1) {
			system("echo \"Invalid Host name or IP address\"");
			log("Invalid traceroute destination!  ", ipaddress);
			system("echo \"##END##\"");

			return;
		}
	}
	if (dest == 1) {
		//char *destination = getDestination(command);
		int i = strcmp(ipaddress, destination);
		int j = strcmp(hostname, destination);
		if (i != 0 && j != 0) {
			system("echo \"users are only allowed to send traceroutes to their own IP addresses\"");
			log(" user tried to traceroute to other IP addresse when STRICT_DEST is true. discarded the command.  ", ipaddress);
			system("echo \"##END##\"");

			return;
		}
	}

	int ret = system(command);
	log("trace route information sent to client", ipaddress);
}

// write entries to log file
void log(const char *logentry, char * ipaddress) {

	/* logs information about followings
	 Connection information
	 1. server connection information.

	 Errors
	 1. Invalid port number.				2. Socket opening errors.
	 3. Socket binding errors.			4. Accepting request errors.
	 5. Max number of users exceeds.		6. Reading data from client errors.

	 Issued Commands
	 1.trace route [destination] 		2. traceroute me
	 3. help								4. quit
	 5. Invalid commands
	 */
	FILE *file;
	file = fopen("log.txt", "a");
	if (!file) {
		system("echo \"Error opening log file\"");
		exit(1);
	} else {
		char etryarray[1000];
		struct tm *myTime;
		char chrDate[20];
		time_t mytm;

		time(&mytm);
		myTime = localtime(&mytm);

		strftime(chrDate, 20, "%m/%d/%Y %H:%M:%S", myTime);

		strcpy(etryarray, chrDate);
		strcat(etryarray, " : ");
        if (ipaddress != NULL) {
            strcat(etryarray, ipaddress);
            strcat(etryarray, " : ");
        }
            
		strcat(etryarray, logentry);
		fprintf(file, "%s\n", etryarray);
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
	log(chrDate, NULL);
}

// prints the error and exits
void error_exit(const char *msg) {
	perror(msg);
	exit(1);
}

// checks if the entered host name is a valid host name using regular expression
// regular expression references - http://stackoverflow.com/questions/106179/regular-expression-to-match-hostname-or-ip-address

int validatehostname(char * hostname) {
	regex_t regex;
	regmatch_t match;
	int reti;

	/* Compile regular expression */
	reti =
			regcomp(&regex,
					"^(([a-zA-Z]|[a-zA-Z][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z]|[A-Za-z][A-Za-z0-9\\-]*[A-Za-z0-9])$",
					REG_EXTENDED);

	/* Execute regular expression */
	reti = regexec(&regex, hostname, 0, NULL, 0);
	if (reti == 0) {
		regfree(&regex);
		return 1;
	} else {
		regfree(&regex);
		return -1;
	}
}

// checks if the entered ip address is a valid ip address using regular expression
// regular expression references - http://stackoverflow.com/questions/106179/regular-expression-to-match-hostname-or-ip-address
int validateipaddress(char * ipaddress) {
	regex_t regex;
	int reti;

	/* Compile regular expression */
	reti =
			regcomp(&regex,
					"^([1-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(\\.([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3}$",
					REG_EXTENDED);

	/* Execute regular expression */
	reti = regexec(&regex, ipaddress, 0, NULL, 0);
	if (reti == 0) {
		regfree(&regex);
		return 1;
	} else {
		regfree(&regex);
		return -1;
	}
}

char * trim (char *s)
{
    int i;

    while (isspace (*s)) s++;   // skip left side white spaces
    for (i = strlen (s) - 1; (isspace (s[i])); i--) ;   // skip right side white spaces
    s[i + 1] = '\0';
    return s;
}
