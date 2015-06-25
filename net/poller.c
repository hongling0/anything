#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>


struct node{
  struct node* next;
  uint32_t id;
  int fd;
  int flag;
  void(*on_event)(struct node*,int event);
};

struct poller {
  int efd;
  struct node* node_hash;
  unsigned node_max;
  unsigned node_now;
};

static uint32_t id_hash(uint32_t key){
  key+=~(key<<15);
  key^=(key>>10);
  key+=(key<<3);
  key^=(key>>6);
  key+=~(key<<11);
  key^=(key>>16);
  return key;
}


struct poller* poller_create() {
  struct poller * p=(struct poller *)malloc(sizeof(*p));
  p->node_max=1024;
  p->node_now=0;
  p->node_hash=(struct node*)malloc(sizeof(*p->node_hash)*p->node_max);
  memset(p->node_hash,0,sizeof((*p->node_hash)*p->node_max));
  p->efd=epoll_create(1);
  if(p->efd==-1){
    fprintf(stderr,"%s failure! %s\n",__func__,strerror(errno));
    free(p);
    return NULL;
  }
  return p;
}

void poller_delete(struct poller * p){
  close(p->efd);
  free(p->node_hash);
  free(p);
}

int poller_addnode(struct poller*p,struct node* n){
  struct epoll_event e;
  e.events=n->flag;
  e.data.ptr=n;
  if(epoll_ctl(p->efd,EPOLL_CTL_ADD,n->fd,&e)==-1){
    fprintf(stderr,"%s failure! %s\n",__func__,strerror(errno));
    return -1;
  }

  uint32_t hash=id_hash(n->id)^(p->node_max-1);
  n->next=p->node_hash[hash];
  p->node_hash[hash]=n;
  return 0;
}

int poller_removenode(struct *p,struct node* n){
  int32_t hash=id_hash(n->id)^(p->node_max-1);
  struct node* head=p->node_hash[hash];
  struct node* prev,cur;
  for(cur=head;cur;prev=cur,cur=cur->next){
    if(cur==n){
      break;
    }
  }
  if(cur!=n){
    return -1;
  }
  if(cur==head){
    p->node_hash[hash]=cur->next;
  }else{
    prev->next=cur->next;
  }
  cur->next=NULL;

  struct epoll_event e;
  e.events=n->flag;
  e.data.ptr=n;
  if(epoll_ctl(p->efd,EPOLL_CTL_DEL,n->fd,&e)==-1){
    fprintf(stderr,"%s failure! %s\n",__func__,strerror(errno));
  }
  return 0;
}

int poller_once(struct poller* p){
  struct epoll_event ev[64];
  int ret=epoll_wait(p->efd,ev,64,-1);
  if(ret>0){
    int i=0;
    for(i=0;i<ret;i++){
      struct epoll_event* e=&ev[i];
      struct node* n=e.data.ptr;
      n->on_event(n,e->events);
    }
  }
}

static int open_listen(const char* host,int port,int backlog){
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

