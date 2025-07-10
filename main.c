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
#include <list.h>

#define MAXBUFSIZE 30
#define BACKLOG 10

LIST *receivelist, *sendlist;
pthread_mutex_t sendmut, receivemut;
pthread_cond_t sendcv, receivecv;
int sockfd, confd;

void *input(void *arg) {
  char msg[30];
  while (1) {
    printf("Type a message: ");
    fgets(msg, sizeof(msg), stdin); 
    pthread_mutex_lock(&sendmut);
    ListAppend(sendlist, &msg);
    pthread_mutex_unlock(&sendmut);
  }	
	return 0;
}

void *sender(void *arg) {
  char *msg;
  while (1) {
   pthread_mutex_lock(&sendmut);
   msg = ListTrim(sendlist);
   send(sockfd, msg, 30, NULL);
   pthread_mutex_unlock(&sendmut);
  }
  return 0;	
}

void *receiver(void *arg) {
  return 0;
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[]) {
	pthread_t input_thread, send_thread, receive_thread, output_thread;
	socklen_t sin_size;
	struct addrinfo *p, *q;
	struct addrinfo hints;
	struct addrinfo *servinfo; 
	struct sockaddr_storage their_addr;
	char s[INET6_ADDRSTRLEN];	
	char *msg;
	char *remote_machine, *local_port, *remote_port;
	int status, len;
	int yes = 1;
  
	if (argc != 4) {
		printf("Wrong number of arguments. Usage: ./chatty <local port> \
<remote IP> <remote port>\n");
		return -1;
	}
  
  if (pthread_mutex_init(&sendmut, NULL) != 0) {
    printf("Error: send mutex init failed\n");
    return -1;
  }
	if (pthread_mutex_init(&receivemut, NULL) != 0) {
    printf("Error: receive mutex init failed\n");
    return -1;
  }
  if (pthread_cond_init(&sendcv, NULL) != 0) {
    printf("Error: send CV init failed\n");
    return -1;
  }


  local_port = argv[1];
	remote_machine = argv[2];
	remote_port = argv[3];
  
  receivelist = ListCreate();
  sendlist = ListCreate();  
	
  if (atoi(local_port) < atoi(remote_port)) {
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		if ((status = getaddrinfo(NULL, local_port, &hints, &servinfo)) != 0) {
			fprintf(stderr,"getaddrinfo error: %s\n",gai_strerror(status));
			exit(1);
		}
		
		for (p = servinfo; p != NULL; p = p->ai_next) {
			if ((socketfd = socket(p->ai_family, 
					p->ai_socktype, 
					p->ai_protocol)) == -1) {
				perror("Server: socket");
				continue;
			}
      if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes,
          sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
      }
			if (bind(socketfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
				close(socketfd);
				perror("Server: bind");
				continue;
			}	

			break;
		}
		if (p == NULL) {
			fprintf(stderr, "Server: failed to bind socket\n");
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
		
    sin_size = sizeof their_addr;
    printf("Waiting for connection...\n");
    confd = accept(socketfd, (struct sockaddr *)&their_addr, &sin_size);
    if (confd == -1) {
      perror("accept");
    }
    inet_ntop(their_addr.ss_family,
        get_in_addr((struct sockaddr *)&their_addr),
            s,
        sizeof s);
    printf("Getting chatty with %s\n", s);
	}
	else {
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		
		if ((status = getaddrinfo(remote_machine, remote_port, &hints,
						&servinfo)) != 0) {
			fprintf(stderr,"getaddrinfo error: %s\n",gai_strerror(status));
			exit(1);
		}
		for (q = servinfo; q != NULL; q = q->ai_next) {
			if ((socketfd = socket(q->ai_family,
					q->ai_socktype,
					q->ai_protocol)) == -1) {
				perror("Client: socket");
				continue;
			}
		      	inet_ntop(q->ai_family, get_in_addr((struct sockaddr *)q->ai_addr), 
				  s, sizeof s);
			printf("Client: attempting connection to %s\n", s);

			if (connect(socketfd, q->ai_addr, q->ai_addrlen) == -1) {
			  perror("Client: connect");
			  close(socketfd);
			  continue;
      			}
			break;
		}

		if (q == NULL) {
			fprintf(stderr, "Client: failed to create socket\n");
			return 2;
		}
		else {
			servinfo = q;
		}
	  printf("Connected!\n");
  }
	pthread_create(&input_thread, NULL, input, (void*) msg);
	pthread_join(input_thread, NULL);
  
  close(socketfd);
  close(confd);
	return 0;
}

