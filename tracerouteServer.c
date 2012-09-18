#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#define PORT "3497"  // the portPORT users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void writeToFile()
{
	FILE *file;
	file = fopen("file.txt","w+"); //apend file (add text to a file or create a file if it does not exist.
	fprintf(file,"%s","This is just an example 2:)"); //writes
	fclose(file); //done!
	exit(1);
}

void ReadFromFile()
{

	FILE *fr;
	int n;
	long elapsed_seconds;
	char line[80];
   

	fr = fopen ("file.txt", "rwb");  // open the file for reading 

	while(fgets(line, 80, fr) != NULL)
	{
		 // get a line, up to 80 chars from fr.  done if NULL 
		 sscanf (line, "%ld", &elapsed_seconds);
		 // convert the string to a long int 
		 printf (line,"%ld\n", elapsed_seconds);
	}
   	fclose(fr);  // close the file prior to exiting the routine 
}



int main (int argc, char** argv)
{
	char *portNumber;
	char *numRequests;
	char *numSecs;
	char *numUsers;
	char *dest;

	 // getting input argumets through command line
	 if ( argc != 11) {
	 	//printf("Usage:\n %s <port number> <number requests> <number seconds> <number of users> <0 or1>\n",argv[0]);
		portNumber = "12";
		numRequests = "14";
		numSecs = "16";
		numUsers = "18";
		dest = "20";
	    } 
	 else {
		
	 	portNumber = argv[2]; 
		numRequests = argv[4]; 
		numSecs = argv[6]; 
		numUsers = argv[8];
	 	dest = argv[10]; 
		}

	printf( "\n PORT: %s ", portNumber );
 	printf( "\n RATE: %s ", numRequests);
	printf( "\n RATE: %s ", numSecs);
 	printf( " \n MAX_USERS: %s ", numUsers );
 	printf( "\n STRICT_DEST: %s", dest );
	printf("\n ");

		   
/*	
	char *samplehostt;
	 samplehostt = "aaaaaaaa";
	char str[] = "aa@aa1aaa";
	int i=0;

	while (str[i])
	  { 

		if(!isalpha(str[i]))   
		{
		  if(!isdigit(str[i]))
		  { 	
			 if(strcmp(str[i], ".") != 0)
			{ 
				if (strcmp(str[i], "-")!= 0)
				{
					printf( "\n InvalidHost" );
				}
			}
		  } 
		 
		}
	    i++;  
	  }

*/

    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP


    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

   

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
 	int tr=1;
    	// kill "Address already in use" error message
    	if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&tr,sizeof(int)) == -1) {
    	perror("setsockopt");
    	exit(1);
    	}

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop

	
        sin_size = sizeof their_addr;

        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            if (send(new_fd, "Hello, world!", 13, 0) == -1)
                perror("send");
		ReadFromFile();
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
	
	usleep(10000); 
printf("server: done...\n");
    }

close(sockfd);
    return 0;

}
