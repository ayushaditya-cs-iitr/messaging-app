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

 
#define bufsize 1000

 
pthread_mutex_t mutex;

struct client{
    char *name;
    int confd;
    struct client *next;
};

struct client *header=NULL;

 

void add_user(struct client *user){

   if(header == NULL){
     header=user;
     user->next=NULL;
   }
   else{
      user->next=header;

      header=user;
   }
}
 
void delete_user(confd){
   struct client *user=header;
   struct client *previous=NULL;
   // identify the user
   while(user->confd!=confd){
     previous=user;
     user=user->next;
   }

   if(previous == NULL)
      header=user->next;

   else
     previous->next=user->next;

    
   free(user->name);
   free(user);

}

 
int connection(char * port){

   struct addrinfo *p, *listp, hints;
   int rc,listenfd,optval=1;

   //initialise to zero
   memset(&hints,0,sizeof(struct addrinfo));
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM; 
   hints.ai_flags =AI_ADDRCONFIG|AI_PASSIVE;
   hints.ai_flags |= AI_NUMERICSERV;  

   if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
     fprintf(stderr,"get_address info failed port number %s is invalid\n",port);
     return -1;
  }

    
   for (p = listp; p; p = p->ai_next) {

       listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
       if (listenfd < 0) {
         continue;  
       }

        
      setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,sizeof(int));
       
      if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) {
          break; 
      }
      if (close(listenfd) < 0) {  
            fprintf(stderr, "open_listenfd close failed: %s\n",
                    strerror(errno));
            return -1;
        }

    }

     
    freeaddrinfo(listp);
    if (!p) {  
        return -1;
    }

     
    if (listen(listenfd, 1024) < 0) {
            close(listenfd);
            return -1;
        }

      return listenfd;
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

void decrypt(char* msg)
{
 
        if(strlen(msg)>0)
        {
                int i=0;
                int length = strlen(msg);
                while(i<length)
                {
                        if(msg[i]!='\n')
                                msg[i] = msg[i] - 1;
                        i++;
                }
 
        }
}

void send_msg(int confd,char* msg, char* receiver, char* sender){

    char response[bufsize];
     
    struct client *user=header;
    if(receiver == NULL)
     while (user != NULL){
      if (user->confd == confd)
      {
         strcpy(response,"msg sent\n\r\n");
	 encrypt(response);
         rio_writen(user->confd,response,strlen(response));
       }

      else{
         sprintf(response,"start\n%s:%s\n\r\n",sender,msg);
	 encrypt(response);
         rio_writen(user->confd,response,strlen(response));
      }
      user=user->next;
    }
   else{
       while (user != NULL){
         if(!strcmp(user->name,receiver)){
           sprintf(response,"start\n%s:%s\n\r\n",sender,msg);
	   encrypt(response);
           rio_writen(user->confd,response,strlen(response));
           strcpy(response,"msg sent\n\r\n");
	   encrypt(response);
           rio_writen(confd,response,strlen(response));
           return;
         }
         user=user->next;
       }
        strcpy(response,"user not found\n\r\n");
	encrypt(response);
        rio_writen(confd,response,strlen(response));

   }

}

void evaluate(char *buf ,int confd ,char *username){

  char response[bufsize];
  char msg[bufsize];
  char receiver[bufsize];
  char keyword[bufsize];
   
  msg[0]='\0';
  receiver[0]='\0';
  keyword[0]='\0';
  struct client *user=header;


  if(!strcmp(buf,"help")){
        sprintf(response,"msg \"text\" : send the msg to all the clients online\n");
        sprintf(response,"%smsg \"text\" user :send the msg to a particular client\n",response);
        sprintf(response,"%sonline : get the username of all the clients online\n",response);
        sprintf(response,"%squit : exit the chatroom\n\r\n",response);
        encrypt(response);
        rio_writen(confd,response,strlen(response));
        return;
   }
    
   if (!strcmp(buf,"online")){
        
        response[0]='\0';
        
        pthread_mutex_lock(&mutex);
        while(user!=NULL){
        sprintf(response,"%s%s\n",response,user->name);
        user=user->next;

        }
    sprintf(response,"%s\r\n",response);
    
    pthread_mutex_unlock(&mutex);
    encrypt(response);
    rio_writen(confd,response,strlen(response));
    return;
   }

   if (!strcmp(buf,"quit")){
      pthread_mutex_lock(&mutex);
      delete_user(confd);
      pthread_mutex_unlock(&mutex);
      strcpy(response,"exit\n");
      encrypt(response);
      rio_writen(confd,response,strlen(response));
      close(confd);
      return;

   }

   sscanf(buf,"%s \" %[^\"] \"%s",keyword,msg,receiver);

   if (!strcmp(keyword,"msg")){

        pthread_mutex_lock(&mutex);
        send_msg(confd,msg,receiver,username);
        pthread_mutex_unlock(&mutex);
   }
  else {
     strcpy(response,"Invalid command\n\r\n");
     encrypt(response);
     rio_writen(confd,response,strlen(response));

  }

}
 
void* client_handler(void *vargp ){

  char username[bufsize];
  rio_t rio;
  struct client *user;
  long byte_size;
  char buf[bufsize];
   
   pthread_detach(pthread_self());

   
   int confd = *((int *)vargp);
   rio_readinitb(&rio, confd);

    
    if( (byte_size=rio_readlineb(&rio,username,bufsize)) == -1){
         close(confd);
         free(vargp);
         return NULL;
    }
     
    username[byte_size-1]='\0';
     
    user=malloc(sizeof(struct client));
     
    if (user == NULL){
      perror("memory can't be assigned");
      close(confd);
      free(vargp);
      return NULL;
    }
     
    user->name=malloc(sizeof(username));
    memcpy(user->name,username,strlen(username)+1);
    user->confd=confd;

     
    pthread_mutex_lock(&mutex);
    add_user(user);
     
    pthread_mutex_unlock(&mutex);

     
    while((byte_size=rio_readlineb(&rio,buf,bufsize)) >0)
    {
        
        buf[byte_size-1]='\0';
         
	decrypt(buf);
        evaluate(buf,confd,username);

    }

    return NULL;
}

int main(int argc,char **argv){
  struct sockaddr_storage clientaddr;
  socklen_t clientlen;
  int listen=-1;
  char host[1000];
  char *port="80";
  int *confd;
  pthread_t tid;

  if (argc > 1)
    port=argv[1];

  
  listen= connection(port);

   
  if(listen == -1){
   printf("connection failed\n");
   exit(1);
  }

  printf("waiting at localhost and port '%s' \n",port);

   
  while(1){
     
      confd=malloc(sizeof(int));
      *confd=accept(listen, (struct sockaddr *)&clientaddr, &clientlen);
      printf("A new client is online\n");
       
       pthread_create(&tid,NULL,client_handler, confd);

  }

}