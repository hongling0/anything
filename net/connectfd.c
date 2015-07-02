#include <assert.h>
#include <error.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "poller.h"


static int open_connect(const char* host,int port){
  struct addrinfo ai;
  struct addrinfo * ai_list=NULL,*ai_ptr=NULL;
  memset(&ai,0,sizeof(ai));
  ai.ai_family = AF_UNSPEC;
  ai.ai_socktype = SOCK_STREAM;
  ai.ai_protocol = IPPROTO_TCP;

  char service[16];
  sprintf(service,"%d",port);

  int status=getaddrinfo(host,service,&ai,&ai_list);
  if(status!=0){
    fprintf(stderr,"%s getaddrinfo failed! %s\n",__func__,gai_strerror(status));
    goto failed;
  }
  int sock=-1;
  for(ai_ptr=ai_list;ai_ptr!=NULL;ai_ptr=ai_ptr->ai_next){
    sock=socket(ai_ptr->ai_family,ai_ptr->ai_socktype,ai_ptr->ai_protocol);
    if(sock<0){
      continue;
    }
    int keepalive=1;
    setsockopt(sock,SOL_SOCKET,SO_KEEPALIVE,(void*)&keepalive,sizeof(keepalive));
    int flag=fcntl(sock,F_GETFL,0);
    if(flag==-1){
      goto sockerr;
    }
    flag=fcntl(sock,F_SETFL,flag|O_NONBLOCK);
    if(flag==-1){
      goto sockerr;
    }
    statuc=connect(sock,ai_ptr->ai_addr,ai_ptr->ai_addrlen);
    if(status!=0&&errno!=EINPROGRESS){
      close(sock);
      sock=-1;
      continue;
    }
    break;
  }
  freeaddrinfo(ai_list);
  return sock;
sockerr:
  close(sock);
failed:
  freeaddrinfo(ai_list);
  return -1;
}
