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
#include <ncurses.h>

#define MAXBUFSIZE 29
#define BACKLOG 10

LIST *receivelist, *sendlist;
pthread_t input_thread, send_thread, receive_thread, output_thread;
pthread_mutex_t sendmut, receivemut, sendcvmut, receivecvmut;
pthread_cond_t sendcv, receivecv;
int sockfd, confd, client;
char *remote_machine, *local_port, *remote_port;
int row, col;
char s[INET6_ADDRSTRLEN];
char closed[] = "Connection closed.";

/* Accept input from the user and add it to the send list */
void *input(void *arg) {
  char *msg;
  while (1) {
    msg = malloc(128);
    pthread_mutex_unlock(&sendmut);
    memset(msg, 0, 128);
    mvprintw(row - 1, 0, "Type a message: ");
    getstr(msg);
    ListAppend(sendlist, msg);
    pthread_mutex_unlock(&sendmut);
    if (ListCount(sendlist) == 1) {
      pthread_cond_signal(&sendcv);
    }
    move(0, 0);
    deleteln();
    move(row - 2, 0);
    deleteln();
    mvprintw(row - 2, 0, "You: %s", msg);
    mvprintw(row - 1, 0, "Type a message: ");
    refresh();
  }	
	return 0;
}
/*
void *output(void *arg) {
  char msg[128];
  while (1) {
    memset(msg, 0, 128);
    pthread_mutex_lock(&receivemut);
    if (ListCount(receivelist) != 0) {
      ListFirst(receivelist);
      strcpy(msg, ListCurr(receivelist));
      ListRemove(receivelist);
      printf("%s\n", msg); 
    }
    pthread_mutex_unlock(&receivemut);
  }

  return 0;
}
*/

/* Take an item off the send list and send it to the other user */
void *sender(void *arg) {
  char *msg;
  int bytes;
  char error[] = "pthread sender exit";
  
  while (1) {
    /* Check if the list has any content to send 
     * If not, wait for some */
    if (ListCount(sendlist) == 0) {
      pthread_mutex_lock(&sendcvmut);
      pthread_cond_wait(&sendcv, &sendcvmut);
      pthread_mutex_unlock(&sendcvmut);
    }
    msg = malloc(128);
    /* Obtain mutex, get and then remove message 
     * from the list, send it */
    pthread_mutex_unlock(&sendmut);
    ListFirst(sendlist);
    strcpy(msg, (char*)ListCurr(sendlist));
    ListRemove(sendlist);
    bytes = send(confd, msg, 128, 0);
    
    /* Check if the message was the command to close the connection */
    if ((strncmp(msg, "/c", 2)) == 0) {
      pthread_cancel(receive_thread);
      pthread_cancel(input_thread);
      pthread_exit(&closed);
    }
    if (bytes == -1) {
      perror("send");
      pthread_exit(&error);
    }
    pthread_mutex_unlock(&sendmut);
  }
  return 0;	
}

/* Receive messages and print them */
void *receiver(void *arg) {
  char *buf;
  int bytes;
  char error[] = "pthread receiver exit";
  while (1) {
    buf = malloc(128);
    memset(buf, 0, 128);
    bytes = recv(confd, buf, 128, 0);
    buf[bytes] = '\0';
    if ((strncmp(buf, "/c", 2)) == 0) {
      pthread_cancel(send_thread);
      pthread_exit(&closed);
    }
    if (bytes != -1) {
      move(0, 0);
      deleteln();
      move(row - 2, 0);
      deleteln();
      mvprintw(row - 2, 0, "%s: %s", s, buf);
      mvprintw(row - 1, 0, "Type a message: ");
      refresh();
    }
    else if (bytes == 0) {
      pthread_exit("connection closed");
    }
    else {
      perror("recv");
      pthread_exit(&error);
    }
  }

  return 0;
}

/* Function to get sockaddr IPv4 or 6. Written by Beej */
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[]) {
	socklen_t sin_size;
	struct addrinfo *p, *q;
	struct addrinfo hints;
	struct addrinfo *servinfo; 
	struct sockaddr_storage their_addr;
	int status;
	int yes = 1;
	
  if (argc != 4) {
		printf("Wrong number of arguments. Usage: ./chatty <local port> \
<remote IP> <remote port>\n");
		return -1;
	}

  /* Mutex and CV init
   * These will be used to coordinate the threads */
  if (pthread_mutex_init(&sendmut, NULL) != 0) {
    fprintf(stderr, "Error: send mutex init failed\n");
    return -1;
  }
	if (pthread_mutex_init(&receivemut, NULL) != 0) {
    fprintf(stderr, "Error: receive mutex init failed\n");
    return -1;
  }
  if (pthread_mutex_init(&receivecvmut, NULL) != 0) {
    fprintf(stderr, "Error: receive cv mutex init failed\n");
    return -1;
  }
 if (pthread_mutex_init(&sendcvmut, NULL) != 0) {
    fprintf(stderr, "Error: send cv mutex init failed\n");
    return -1;
  } 
  if (pthread_cond_init(&sendcv, NULL) != 0) {
    fprintf(stderr, "Error: send CV init failed\n");
    return -1;
  }
  if (pthread_cond_init(&receivecv, NULL) != 0) {
    fprintf(stderr, "Error: receive CV init failed\n");
    return -1;
  }

  local_port = argv[1];
	remote_machine = argv[2];
	remote_port = argv[3];
  
  /* Linked lists to hold messages to be sent and printed */
  receivelist = ListCreate();
  sendlist = ListCreate();  
	
  /* Decide who is "host" and who is "client" */
  if (atoi(local_port) < atoi(remote_port)) {
    client = 0;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		if ((status = getaddrinfo(NULL, local_port, &hints, &servinfo)) != 0) {
			fprintf(stderr,"getaddrinfo error: %s\n",gai_strerror(status));
			exit(1);
		}
		
		for (p = servinfo; p != NULL; p = p->ai_next) {
			if ((sockfd = socket(p->ai_family, 
					p->ai_socktype, 
					p->ai_protocol)) == -1) {
				perror("Server: socket");
				continue;
			}
      if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
          sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
      }
			if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
				close(sockfd);
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
		if (listen(sockfd, BACKLOG) == -1) {
			perror("listen");
			exit(1);
		}
		
    sin_size = sizeof their_addr;
    printf("Waiting for connection...\n");
    confd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
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
    client = 1;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		
		if ((status = getaddrinfo(remote_machine, remote_port, &hints,
						&servinfo)) != 0) {
			fprintf(stderr,"getaddrinfo error: %s\n",gai_strerror(status));
			exit(1);
		}
		for (q = servinfo; q != NULL; q = q->ai_next) {
			if ((confd = socket(q->ai_family,
					q->ai_socktype,
					q->ai_protocol)) == -1) {
				perror("Client: socket");
				continue;
			}
		      	inet_ntop(q->ai_family, get_in_addr((struct sockaddr *)q->ai_addr), 
				  s, sizeof s);
			printf("Connecting to %s\n", s);

			if (connect(confd, q->ai_addr, q->ai_addrlen) == -1) {
			  perror("Client: connect");
			  close(sockfd);
			  continue;
      			}
			break;
		}

		if (q == NULL) {
			fprintf(stderr, "Client: failed to create socket\n");
			return 2;
		}
    servinfo = q;

  }

  initscr();
  cbreak();
  getmaxyx(stdscr, row, col);
	keypad(stdscr, TRUE);
 
  pthread_create(&input_thread, NULL, input, NULL);
  pthread_create(&send_thread, NULL, sender, NULL);
	pthread_create(&receive_thread, NULL, receiver,  NULL);
  
  pthread_join(send_thread, NULL);
  pthread_join(receive_thread, NULL);
  pthread_cond_destroy(&sendcv);
  pthread_cond_destroy(&receivecv);
  pthread_mutex_destroy(&sendcvmut);
  pthread_mutex_destroy(&receivecvmut);
  pthread_mutex_destroy(&sendmut);
  pthread_mutex_destroy(&receivemut);
  
  close(sockfd);
  close(confd);
  refresh();
  endwin();
	printf("Connection closed.\n");
  return 0;
}

