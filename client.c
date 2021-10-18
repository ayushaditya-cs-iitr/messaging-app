#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "helper.h"

#define MAXLINE 1024 

char prompt[]="Chatroom> ";
int flag=0;
 
void usage(){
  printf("-h  print help\n");
  printf("-a  IP address of the server[Required]\n");
  printf("-p  port number of the server[Required]\n");
  printf("-u  enter your username[Required]\n");
}

 

int connection(char* hostname, char* port){
  int clientfd,rc;
  struct addrinfo hints, *listp, *p;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;  
  hints.ai_flags |=AI_ADDRCONFIG;
  hints.ai_flags |= AI_NUMERICSERV; 


  if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0) {
    fprintf(stderr,"invalid hostname or port number\n");
    return -1;
 }

 for (p = listp; p; p = p->ai_next) {
       
        clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (clientfd < 0) {
            continue;  
        }

        
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1) {
            break;  
        }

        
        if (close(clientfd) < 0) {
            fprintf(stderr, "open_clientfd: close failed: %s\n",
                    strerror(errno));
            return -1;
        }
    }

    
    freeaddrinfo(listp);
    if (!p) {  
            return -1;
    }
    else {  
            return clientfd;
    }
}
void encrypt(char* msg)
{
 
        if(strlen(msg)>0)
        {
                int i=0;
                int length = strlen(msg);
                while(i<length)
                {
                        if(msg[i]!='\n')
                                msg[i] = msg[i] + 1;
                        i++;
                }
 
        }
}

void decrypt(char *msg)
{
 
        if(strlen(msg)>0)
        {
                int i=0;
                int length = strlen(msg);
                while(i<length-1)
                {
                        msg[i] = msg[i] - 1;
                        i++;
                }
          
        }
}

 
void reader(void* var){
  char buf[MAXLINE];
  rio_t rio;
  int status;
  int connID=(int)var;
   
  rio_readinitb(&rio, connID);
  while(1){
     while((status=rio_readlineb(&rio,buf,MAXLINE)) >0){
         
          if(status == -1)
            exit(1);
          if(!strcmp(buf,"\r\n")){
              break;
            }
	  decrypt(buf);
         
          if (!strcmp(buf,"exit\n")){
              close(connID);
              exit(0);
            }
          if (!strcmp(buf,"start\n")){

               printf("\n");
            }

          else
	  {	
	 
             	  printf("%s",buf);
	  }
	  

      }
       
      fflush(stdout);
  }
}

int main(int argc, char **argv){


  char *address=NULL,*port=NULL,*username=NULL;
  char cmd[MAXLINE];
  char c;
  pthread_t tid;
   
  while((c = getopt(argc, argv, "hu:a:p:u:")) != EOF){
    switch(c){
       
      case 'h':
         usage();
         exit(1);
         break;
      
      case 'a':
         address=optarg;
         break;
      
      case 'p':
         port=optarg;
         break;
      
      case 'u':
         username=optarg;
         break;

      default:
          usage();
          exit(1);

    }


   }

   if(optind  == 1 || port == NULL || address == NULL || username == NULL){
    printf("Invalid usage\n");
    usage();
    exit(1); }

    int connID=connection(address,port);
    if(connID == -1){
       printf("Couldn't connect to the server\n");
       exit(1);
    }
    
    sprintf(username,"%s\n",username);

    
    if(rio_writen(connID,username,strlen(username)) == -1){
       perror("not able to send the data");
       close(connID);
       exit(1);
    }

     
    pthread_create(&tid,NULL,reader, (void*)connID);
    
    printf("%s",prompt);
    while(1){
      
      if ((fgets(cmd, MAXLINE, stdin) == NULL) && ferror(stdin)) {
            perror("fgets error");
            close(connID);
            exit(1);
        }

	encrypt(cmd);
      
      if (rio_writen(connID,cmd,strlen(cmd)) == -1){
          perror("not able to send the data");
          close(connID);
          exit(1);
        }

    }

}