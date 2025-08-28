/*
 * A simple chat program by Dane MacFadden
 */
#define MAXBUFSIZE 29
#define BACKLOG 10
#define _XOPEN_SOURCE 700

#include <wchar.h>
#include <locale.h>
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
#include <notcurses/notcurses.h>

/* threading stuff */
LIST *receivelist, *sendlist;
pthread_t input_thread, send_thread, receive_thread, output_thread;
pthread_mutex_t sendmut, receivemut, sendcvmut, receivecvmut, rendermut;
pthread_cond_t sendcv, receivecv;
char closed[] = "Connection closed.";

/* networking stuff */
int confd, sockfd;
char s[INET6_ADDRSTRLEN];

/* notcurses stuff */
struct notcurses *nc;
struct ncplane *chatlog, *prompt, *stdn;
unsigned int rows, cols;

/* Accept input from the user and add it to the send list */
void *input() {
  char *msg;
  struct ncinput ni;
  uint32_t character;
  int i;
  
  while (1) {
    /* Get a message from the user */
    msg = malloc(512);
    memset(msg, 0, 512);
    i = 0;
    ncplane_printf_yx(prompt, 0, 0, "Type a message: ");
    
    pthread_mutex_lock(&rendermut);
    notcurses_render(nc);
    pthread_mutex_unlock(&rendermut);
   
    /* Loop to get input */
    while (1) {
      character = notcurses_get(nc, NULL, &ni);  
      if (character == NCKEY_ENTER) {
        /* Add to sendlist when user presses enter */
        pthread_mutex_lock(&sendmut);
        ListAppend(sendlist, msg);
        pthread_mutex_unlock(&sendmut);
       
        ncplane_erase_region(prompt, 0, 16, 1, cols);

        pthread_mutex_lock(&rendermut);
        notcurses_render(nc);
        pthread_mutex_unlock(&rendermut);
   
        break;
      }
      /* Unsure why this doesn't create a scrolling effect */
      else if (character == NCKEY_SCROLL_UP) {
        ncplane_scrollup(chatlog, -1);
      }
      else if (character == NCKEY_SCROLL_DOWN) {
        ncplane_scrollup(chatlog, 1);
      }
      else if (character == NCKEY_BACKSPACE && i > 0) {
        i--;
        msg[i] = '\0';
      }
      else if (i < 511) {
        msg[i++] = (char)character;
        msg[i] = '\0';
      }

      /* Print the message as the user types it */
      ncplane_erase_region(prompt, 0, 0, 1, cols);
      ncplane_printf_yx(prompt, 0, 0, "Type a message: %s", msg);
      
      pthread_mutex_lock(&rendermut);
      notcurses_render(nc);
      pthread_mutex_unlock(&rendermut);
    }
    
    /* Wake up thread if it was waiting to have a msg to send */
    if (ListCount(sendlist) == 1) {
      pthread_cond_signal(&sendcv);
    }
  }	
	return 0;
}

/* Take an item off the send list and send it to the other user */
void *sender() {
  char *msg, *header;
  int bytes;

  while (1) {
    /* Check if the list has any content to send 
     * If not, wait for some to arrive */
    if (ListCount(sendlist) == 0) {
      pthread_mutex_lock(&sendcvmut);
      pthread_cond_wait(&sendcv, &sendcvmut);
      pthread_mutex_unlock(&sendcvmut);
    }
    msg = malloc(512);
    header = malloc(519);
    memset(msg, 0, 512);
    memset(header, 0, 519);
    /* Obtain mutex, get and then remove message 
     * from the list, unlock mutex,
     * send message. */
    pthread_mutex_unlock(&sendmut);
    ListFirst(sendlist);
    strcpy(msg, (char*)ListCurr(sendlist));
    ListRemove(sendlist);
    pthread_mutex_unlock(&sendmut);
    
    bytes = send(confd, msg, 512, 0);
    
    /* Append message to header and add to print list */
    strcpy(header, "You: ");
    strncat(header, msg, strlen(msg));
    pthread_mutex_lock(&receivemut);
    ListAppend(receivelist, header);
    pthread_mutex_unlock(&receivemut);
    
    /* Wake thread waiting for message to print */
    if (ListCount(receivelist) == 1) {
      pthread_cond_signal(&receivecv);
    }

    /* Stop execution if the user closes the connection */
    if ((strncmp(msg, "/c", 2)) == 0 || bytes == -1) {
      pthread_cancel(receive_thread);
      pthread_cancel(input_thread);
      pthread_cancel(output_thread);
      pthread_exit(&closed);
    }
  }
  return 0;	
}

/* Receive messages, add to print queue */
void *receiver() {
  char *buf, *header;
  int bytes;
  
  while (1) {
    buf = malloc(512);
    header = malloc(strlen(s) + 514);
    memset(buf, 0, 512);
    memset(header, 0, strlen(s) + 514);
    strcpy(header, s);
    strncat(header, ": ", 3);
    
    bytes = recv(confd, buf, 512, 0);
    strncat(header, buf, strlen(buf));
    
    /* Stop execution if the user closes the connection */
    if ((strncmp(buf, "/c", 2)) == 0) {
      pthread_cancel(send_thread);
      pthread_cancel(input_thread);
      pthread_cancel(output_thread);
      pthread_exit(&closed);
    }
    
    /* Add message to list for other thread to print */
    if (bytes != -1) {
      pthread_mutex_lock(&receivemut);
      ListAppend(receivelist, header);
      pthread_mutex_unlock(&receivemut);
    }
    
    /* Signal output thread, if it was waiting for something to print */
    if (ListCount(receivelist) == 1) {
      pthread_cond_signal(&receivecv);
    }
  }
  return 0;
}

/* Remove a msg from the receivelist and display it */
void *output() {
  char *msg;
  while (1) {
    msg = malloc(512 + strlen(s));

    /* Block if there's nothing to output */
    if (ListCount(receivelist) == 0) {
      pthread_mutex_lock(&receivecvmut);
      pthread_cond_wait(&receivecv, &receivecvmut);
      pthread_mutex_unlock(&receivecvmut);
    }
    
    /* Remove a message from the list and print it to chatlog */
    pthread_mutex_lock(&receivemut);
    ListFirst(receivelist);
    strcpy(msg, (char*)ListCurr(receivelist));
    ListRemove(receivelist);
    pthread_mutex_unlock(&receivemut);
    
    ncplane_printf(chatlog, "%s\n", msg);
    
    pthread_mutex_lock(&rendermut);
    notcurses_render(nc);
    pthread_mutex_unlock(&rendermut);
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
  char *remote_machine, *local_port, *remote_port;
  socklen_t sin_size;
	struct addrinfo *p, *q;
	struct addrinfo hints;
	struct addrinfo *servinfo; 
	struct sockaddr_storage their_addr;
	int status;
	int yes = 1;
 
  struct notcurses_options opts = {
    .flags = NCOPTION_SUPPRESS_BANNERS
  };
  struct ncplane_options nopts = {
    .y = 0,
    .x = 0,
    .rows = 0,
    .cols = 0
  };
   struct ncplane_options popts = {
    .y = 0,
    .x = 0,
    .rows = 1,
    .cols = 0
  };
     
  if (argc != 4) {
		printf("Wrong number of arguments. Usage: ./chatty <local port> \
<remote IP> <remote port>\n");
		return -1;
	}

  local_port = argv[1];
	remote_machine = argv[2];
	remote_port = argv[3];

  /* Decide who is "host" and who is "client" */
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
			printf("Getting chatty with %s\n", s);

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
 
  /* Mutex and CV init */
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
  if (pthread_mutex_init(&rendermut, NULL) != 0) {
    fprintf(stderr, "Error: render mutex init failed\n");
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

  /* Linked lists to hold messages to be sent and printed */
  receivelist = ListCreate();
  sendlist = ListCreate();  
 
  /* notcurses init */
  if (!setlocale(LC_ALL, "")) {
    return -1;
  }
  if ((nc = notcurses_core_init(&opts, stdout)) == NULL) {
    return -1;
  }
  if ((stdn = notcurses_stdplane(nc)) == NULL) {
    return -1;
  }
 
  notcurses_stddim_yx(nc, &rows, &cols);
  nopts.rows = rows; 
  nopts.cols = cols;
  popts.cols = cols;
  popts.y = rows - 1;
  notcurses_cursor_disable(nc);
 
  if ((chatlog = ncplane_create(stdn, &nopts)) == NULL) {
    return -1;
  }
  if ((prompt = ncplane_create(stdn, &popts)) == NULL) {
    return -1;
  }
  ncplane_set_scrolling(chatlog, true);
  pthread_create(&input_thread, NULL, input, NULL);
  pthread_create(&send_thread, NULL, sender, NULL);
	pthread_create(&output_thread, NULL, output, NULL);
  pthread_create(&receive_thread, NULL, receiver,  NULL);
  
  pthread_join(send_thread, NULL);
  pthread_join(receive_thread, NULL);
  pthread_join(input_thread, NULL);
  pthread_join(output_thread, NULL);

  pthread_cond_destroy(&sendcv);
  pthread_cond_destroy(&receivecv);
  
  pthread_mutex_destroy(&sendcvmut);
  pthread_mutex_destroy(&receivecvmut);
  pthread_mutex_destroy(&sendmut);
  pthread_mutex_destroy(&receivemut);
  
  close(sockfd);
  close(confd);
  ListFree(receivelist);
  ListFree(sendlist);
  notcurses_stop(nc);
  printf("Connection closed.\n");
  return 0;
}
