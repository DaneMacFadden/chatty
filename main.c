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
#include <notcurses/notcurses.h>
#include "context.h"

/* Accept input from the user and add it to the send list */
void *input(void *arg) {
  char *msg;
  struct ncinput ni;
  uint32_t character;
  int i, savedsize;
  CONTEXT *context = (CONTEXT *)arg; 

  while (1) {
    /* This is the message that will be saved to history linked list */
    char *savedmsg;
    /* Get a message from the user */
    msg = malloc(512);
    memset(msg, 0, 512);
    i = 0;
    ncplane_printf_yx(context->plane_prompt, 0, 0, "Type a message: ");
    
    pthread_mutex_lock(&context->mut_render);
    notcurses_render(context->ctx_nc);
    pthread_mutex_unlock(&context->mut_render);
   
    /* Loop to get input */
    while (1) {
      character = notcurses_get(context->ctx_nc, NULL, &ni);  
      if (character == NCKEY_ENTER) {
        /* Add to sendlist when user presses enter */
        pthread_mutex_lock(&context->mut_send);
        ListAppend(context->ctx_sendlist, msg);
        pthread_mutex_unlock(&context->mut_send);
       
        ncplane_erase_region(context->plane_prompt, 0, 16, 1, 
        context->ctx_cols);

        pthread_mutex_lock(&context->mut_render);
        notcurses_render(context->ctx_nc);
        pthread_mutex_unlock(&context->mut_render);
   
        break;
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
      ncplane_erase_region(context->plane_prompt, 0, 0, 1, context->ctx_cols);
      ncplane_printf_yx(context->plane_prompt, 0, 0, "Type a message: %s", msg);
      
      pthread_mutex_lock(&context->mut_render);
      notcurses_render(context->ctx_nc);
      pthread_mutex_unlock(&context->mut_render);
    }
    
    /* Wake up thread if it was waiting to have a msg to send */
    if (ListCount(context->ctx_sendlist) == 1) {
      pthread_cond_signal(&context->cv_send);
    }
  }	
	return 0;
}

/* Take an item off the send list and send it to the other user */
void *sender(void *arg) {
  char *msg, *header, *savedmsg;
  int bytes;
  CONTEXT *context = (CONTEXT *)arg; 
  
  while (1) {
    /* Check if the list has any content to send 
     * If not, wait for some to arrive */
    if (ListCount(context->ctx_sendlist) == 0) {
      pthread_mutex_lock(&context->mut_sendcv);
      pthread_cond_wait(&context->cv_send, &context->mut_sendcv);
      pthread_mutex_unlock(&context->mut_sendcv);
    }
    
    msg = malloc(512);
    header = malloc(519);
    memset(msg, 0, 512);
    memset(header, 0, 519);
    
    /* Obtain mutex, get and then remove message 
     * from the list, unlock mutex,
     * send message. */
    pthread_mutex_unlock(&context->mut_send);
    ListFirst(context->ctx_sendlist);
    strcpy(msg, (char*)ListCurr(context->ctx_sendlist));
    ListRemove(context->ctx_sendlist);
    pthread_mutex_unlock(&context->mut_send);
    
    bytes = send(context->ctx_confd, msg, 512, 0);
    
    /* Stop execution if the user closes the connection */
    if ((strncmp(msg, "/c", 2)) == 0 || bytes == -1) {
      pthread_cancel(*context->thread_in);
      pthread_cancel(*context->thread_out);
      pthread_cancel(*context->thread_recv);
      pthread_exit(&context->ctx_closed);
    }
    
    /* Append message to header and add to print list */
    strcpy(header, "You: ");
    strncat(header, msg, strlen(msg));
    
    pthread_mutex_lock(&context->mut_recv);
    ListAppend(context->ctx_recvlist, header);
    pthread_mutex_unlock(&context->mut_recv);
    
    /* Wake thread waiting for message to print */
    if (ListCount(context->ctx_recvlist) == 1) {
      pthread_cond_signal(&context->cv_recv);
    }
  }
  return 0;	
}

/* Receive messages, add to print queue */
void *receiver(void *arg) {
  char *msg, *header;
  int bytes;
  CONTEXT *context = (CONTEXT *)arg;

  while (1) {
    msg = malloc(512);
    header = malloc(strlen(context->ip) + 514);
    memset(msg, 0, 512);
    memset(header, 0, strlen(context->ip) + 514);
    
    bytes = recv(context->ctx_confd, msg, 512, 0);
    
    /* Stop execution if the user closes the connection */
    if ((strncmp(msg, "/c", 2)) == 0) {
      pthread_cancel(*context->thread_send);
      pthread_cancel(*context->thread_in);
      pthread_cancel(*context->thread_out);
      pthread_exit(&context->ctx_closed);    
    }
    
    strncpy(header, context->ip, strlen(context->ip));
    strcat(header, ": ");
    strncat(header, msg, strlen(msg));
    
    /* Add message to list for other thread to print */
    if (bytes != -1) {
      pthread_mutex_lock(&context->mut_recv);
      ListAppend(context->ctx_recvlist, header);
      pthread_mutex_unlock(&context->mut_recv);
    }
    
    /* Signal output thread, if it was waiting for something to print */
    if (ListCount(context->ctx_recvlist) == 1) {
      pthread_cond_signal(&context->cv_recv);
    }
  }
  return 0;
}

/* Remove a msg from the receivelist and display it */
void *output(void *arg) {
  char *msg;
  CONTEXT *context = (CONTEXT *)arg;  

  while (1) {
    msg = malloc(512 + strlen(context->ip));

    /* Block if there's nothing to output */
    if (ListCount(context->ctx_recvlist) == 0) {
      pthread_mutex_lock(&context->mut_recvcv);
      pthread_cond_wait(&context->cv_recv, &context->mut_recvcv);
      pthread_mutex_unlock(&context->mut_recvcv);
    }
    
    /* Remove a message from the list and print it to chatlog */
    pthread_mutex_lock(&context->mut_recv);
    ListFirst(context->ctx_recvlist);
    strcpy(msg, (char*)ListCurr(context->ctx_recvlist));
    ListRemove(context->ctx_recvlist);
    pthread_mutex_unlock(&context->mut_recv);
    
    ncplane_printf(context->plane_log, "%s\n", msg);
    
    pthread_mutex_lock(&context->mut_render);
    notcurses_render(context->ctx_nc);
    pthread_mutex_unlock(&context->mut_render);
    
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
  /* networking stuff */
  int confd, sockfd;
  char s[INET6_ADDRSTRLEN];
  char *remote_machine, *local_port, *remote_port;
  socklen_t sin_size;
	struct addrinfo *p, *q;
	struct addrinfo hints;
	struct addrinfo *servinfo; 
	struct sockaddr_storage their_addr;
	int status;
	int yes = 1;

  /* threading stuff */
  LIST *receivelist, *sendlist, *history;
  pthread_t input_thread, send_thread, receive_thread, output_thread;
  pthread_mutex_t sendmut, 
                  receivemut,
                  sendcvmut,
                  receivecvmut,
                  rendermut,
                  savedmut;
  pthread_cond_t sendcv, receivecv;
  char closed[] = "Connection closed.";

  /* notcurses stuff */
  struct notcurses *nc;
  struct ncplane *chatlog, *prompt, *stdn;
  unsigned int rows, cols;
  
   
  struct notcurses_options opts = {
    .flags = NCOPTION_SUPPRESS_BANNERS
  };
  struct ncplane_options clopts = {
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
			  close(confd);
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
  if (pthread_mutex_init(&savedmut, NULL) != 0) {
    fprintf(stderr, "Error: saved mutex init failed\n");
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
  history = ListCreate();
 
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
  clopts.rows = rows; 
  clopts.cols = cols;
  popts.cols = cols;
  popts.y = rows - 1;
  notcurses_cursor_disable(nc);
 
  if ((chatlog = ncplane_create(stdn, &clopts)) == NULL) {
    return -1;
  }
  if ((prompt = ncplane_create(stdn, &popts)) == NULL) {
    return -1;
  }
  ncplane_set_scrolling(chatlog, true);
  
  CONTEXT context = {
    .ctx_recvlist = receivelist,
    .ctx_sendlist = sendlist,
    
    .thread_in = &input_thread,
    .thread_out = &output_thread,
    .thread_recv = &receive_thread,
    .thread_send = &send_thread,

    .mut_send = sendmut,
    .mut_recv = receivemut,
    .mut_sendcv = sendcvmut,
    .mut_recvcv = receivecvmut,
    .mut_render = rendermut,

    .cv_send = sendcv,
    .cv_recv = receivecv,

    .ip = s,
    .ctx_confd = confd,

    .ctx_nc = nc,
    .plane_log = chatlog,
    .plane_prompt = prompt,
    .ctx_rows = rows,
    .ctx_cols = cols,
    .ctx_closed = closed,
  };

  pthread_create(&input_thread, NULL, input, &context);
  pthread_create(&send_thread, NULL, sender, &context);
	pthread_create(&output_thread, NULL, output, &context);
  pthread_create(&receive_thread, NULL, receiver,  &context);
  
  pthread_join(input_thread, NULL);
  pthread_join(output_thread, NULL);
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
  ListFree(receivelist);
  ListFree(sendlist);
  ListFree(history);
  notcurses_stop(nc);
  printf("%s\n", closed);
  return 0;
}
