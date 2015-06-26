#include <assert.h>
#include <error.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "poller.h"

static void _accept(struct ev_execute* e,struct ev_node* node){
  unsigned char sa[512];
  while(true){
    socklen_t socklen=512;
    int fd=accept(node->fd,(struct sockaddr*)sa,&socklen);
    if(fd==-1){
      int err=errno;
      switch(err){
        case EAGAIN:
        case EWOULDBLOCK:
          break;
        case ECONNABORTED:
        case EMFILE:
        case ENFILE:
        default:{
          fprintf(stderr,"%s failed! %s",__func__,strerror(err));
          return;
        }
      }
    }
  }
}

static void _ev_call(struct ev_execute* e,struct ev_node* n, int ev){
  struct sock_node* sock=(struct sock_node)(n+1);
  if(ev&EPOLLOUT){
    sock->write=1;
    _write(e,n);
  }
  if(ev&EPOLLIN){
    _read(e,n);
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


struct sock_node{
  unsigned write:1;
};

uint32_t start_listen(struct ev_execute* e,int obj,const char* host,int port,int backlog){
  int fd=_listen(host,port,backlog);
  if(fd<0){
    return -1;
  }
  struct ev_node* node=ev_node_create(sizeof(struct listen_node),NULL);
  node->fd=fd;
  node->flag=EPOLLOUT|EPOLLIN|EPOLLET;
  node->call=_ev_call;
  execute_add(e,n);
  return node->id;
}

int stop_listen(struct ev_execute* e,uint32_t id){
  struct ev_node* node=execute_get(e,id);
  assert(node);
  execute_del(node);
  close(node->fd);
  ev_node_delete(node);
}