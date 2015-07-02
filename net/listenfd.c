#include <assert.h>
#include <error.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ev.h"

struct listenfd_node{
  struct fd_node fd;
  struct ev_node read;
};

static void fd_handle(struct ev_execute* e,struct ev_node* n,int events){
  struct listenfd_node* node=(struct listenfd_node*)n;
  struct ev_node* ev;
  if(events&EPOLLIN){
    ev=&node->read;
    if(ev->active){
      ev->enable=1;
      ev->handle(e,ev);
    }
  }
}

int listenfd_accept(struct ev_execute* e,struct fd_node* n){
  unsigned char sa[512];
  socklen_t socklen=512;
  int fd=accept(node->fd,(struct sockaddr*)sa,&socklen);
  if(fd==-1){
    int err=errno;
    switch(err){
      case EAGAIN:
      case EWOULDBLOCK:
        return err;
      case ECONNABORTED:
      case EMFILE:
      case ENFILE:
      default:{
        fprintf(stderr,"%s failed! %s",__func__,strerror(err));
        return err;
      }
    }
  }else{
    return fd;
  }
}

static int _listen(const char* host,int port,int backlog){
  uint32_t addr=INADDR_ANY;
  if(host[0]) {
    addr=inet_addr(host);
  }
  int fd=socket(AF_INET,SOCKET_STREAM,0);
  if(fd<0){
    fprintf(stderr,"%s socket failed! %s\n",__func__,strerror(errno));
    return -1;
  }
  int reuse=1;
  if(setsocketopt(fd,SOL_SOCKET,SO_REUSEADDR,(void*)&reuse,sizeof(reuse))<0){
    fprintf(stderr,"%s setsocketopt failed! %s\n",__func__,strerror(errno));
    goto failed
  }
  struct sockaddr_in addr;
  memset(&addr,0,sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr=addr;
  if(bind(fd,(struct sockaddr*)&addr,sizeof(struct sockaddr))<0){
    fprintf(stderr,"%s bind failed! %s\n",__func__,strerror(errno));
    goto failed;
  }
  if(listen(fd,backlog)<0){
    fprintf(stderr,"%s listen failed! %s\n",__func__,strerror(errno));
    goto failed;
  }
  return fd;
failed:
  close(fd);
  return -1;
}
