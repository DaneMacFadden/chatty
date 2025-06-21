/*
 * A simple chat program by Dane MacFadden
 *
 */

#include <stdio.h>
#include <pthread.h>
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
#include <fcntl.h>

#define MAXBUFSIZE 30
#define BACKLOG 10

void *input(void *arg) {
	while (1) {
	/*
	 * Accept input
	 */
	}	
	return 0;
}

void *sender(void *arg) {
	return 0;	
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[]) {
	pthread_t input_thread, send_thread, receive_thread;
	socklen_t sin_size;
	struct addrinfo *p, *q;
	struct addrinfo hints, remote_hints;
	struct addrinfo *servinfo, *remote_servinfo; 
	struct sockaddr_storage their_addr;
	char s[INET6_ADDRSTRLEN];	
	char *msg;
	char *remote_machine, *local_port, *remote_port;
	int status, socketfd, confd;
	int yes = 1;

	if (argc != 4) {
		printf("Wrong number of arguments\n");
		return -1;
	}

	local_port = argv[1];
	remote_machine = argv[2];
	remote_port = argv[3];

	if (atoi(local_port) < atoi(remote_port)) {
		/* Congrats, you're the server */
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		if ((status = getaddrinfo(NULL, local_port, &hints, &servinfo)) != 0) {
			fprintf(stderr,"getaddrinfo error: %s\n",gai_strerror(status));
			exit(1);
		}
		
		/* Create socket */
		for (p = servinfo; p != NULL; p = p->ai_next) {
			if ((socketfd = socket(p->ai_family, 
					p->ai_socktype, 
					p->ai_protocol)) == -1) {
				perror("listener: socket");
				continue;
			}

			/* Bind */
			if (bind(socketfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
				close(socketfd);
				perror("listener: bind");
				continue;
			}	

			break;
		}
		if (p == NULL) {
			fprintf(stderr, "listener: failed to bind socket\n");
			return 2;
		}
		else {
			servinfo = p;
		}
		
		freeaddrinfo(servinfo);
		if (listen(socketfd, BACKLOG) == -1) {
			perror("listen");
			exit(1);
		}
		
		while (1) {
			sin_size = sizeof their_addr;
			confd = accept(socketfd, (struct sockaddr *)&their_addr, &sin_size);
			if (confd == -1) {
				perror("accept");
				continue;
			}
		inet_ntop(their_addr.ss_family,
			    get_in_addr((struct sockaddr *)&their_addr),
			    s, sizeof s);
		printf("server: got connection from %s\n", s);

		}

	}
	else {
		/* You're the client */
		/* Get remote info */
		memset(&remote_hints, 0, sizeof(remote_hints));
		remote_hints.ai_family = AF_INET;
		remote_hints.ai_socktype = SOCK_STREAM;
		
		if ((status = getaddrinfo(remote_machine, remote_port, &remote_hints,
						&remote_servinfo)) != 0) {
			fprintf(stderr,"getaddrinfo error: %s\n",gai_strerror(status));
			exit(1);
		}
		for (q = remote_servinfo; q != NULL; q = q->ai_next) {
			/* Create socket */
			if ((socketfd = socket(q->ai_family,
					q->ai_socktype,
					q->ai_protocol)) == -1) {
				perror("talker: socket");
				continue;
			}

			break;
		}

		if (q == NULL) {
			fprintf(stderr, "talker: failed to create socket\n");
			return 2;
		}
		else {
			remote_servinfo = q;
		}
	}

	
	pthread_create(&input_thread, NULL, input, (void*) msg);
	
	


	pthread_join(input_thread, NULL);
	return 0;
}

