#include <assert.h>
#include <error.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

struct ev_execute {
  int efd;
  struct ev_node* node_hash;
  unsigned node_max;
  unsigned node_now;
};

uint32_t ev_node_allocid(){
  static uint32_t alloc;
  return alloc++;
}

static uint32_t id_hash(uint32_t key){
  key+=~(key<<15);
  key^=(key>>10);
  key+=(key<<3);
  key^=(key>>6);
  key+=~(key<<11);
  key^=(key>>16);
  return key;
}

static struct ev_node* _hash_del(struct ev_execute* e, struct ev_node *n){
  struct ev_node* head,cur,prev;
  uint32_t hash;

  hash=id_hash(n->id)&(e->node_max-1);
  head=e->node_hash[hash];
  for(cur=head;cur;prev=cur,cur=cur->next){
    if(cur==n){
      break;
    }
  }
  if(cur!=n){
    return NULL;
  }
  if(cur==head){
    e->node_hash[hash]=cur->next;
  }else{
    prev->next=cur->next;
  }
  cur->next=NULL;
  e->node_now--;
  return cur;
}

static struct ev_node* _hash_find(struct ev_execute* e, struct ev_node *n){
  struct ev_node* head,cur,prev;
  uint32_t hash;

  hash=id_hash(n->id)&(e->node_max-1);
  head=e->node_hash[hash];
  for(cur=head;cur;prev=cur,cur=cur->next){
    if(cur==n){
      break;
    }
  }
  if(cur!=n){
    return NULL;
  }
  return n;
} 

static void _hash_add(struct ev_execute* e,struct ev_node* n){
  uint32_t hash=id_hash(n->id)&(e->node_max-1);
  n->next=e->node_hash[hash];
  e->node_hash[hash]=n;
  e->node_now++;
}

static void _rehash(struct ev_execute* e){
  struct ev_node* node_hash,*node,*next;
  unsigned node_max,i;

  node_max=e->node_max;
  node_hash=e->node_hash;
  e->node_now=0;
  e->node_max=node_max>0?node_max*2:1024;
  e->node_hash=(struct ev_node*)malloc(sizeof(*e->node_hash)*e->node_max);
  memset(e->node_hash,0,sizeof((*e->node_hash)*e->node_max));
  for(i=0;i<e->node_max;i++){
    node=node_hash[i];
    for(;node;node=next){
      next=node->next;
      _hash_add(e,node);
    }
  }
}

struct ev_execute* execute_create() {
  struct ev_execute * e=(struct ev_execute *)malloc(sizeof(*e));
  e->node_max=0;
  e->node_now=0;
  _rehash(e);
  e->efd=epoll_create(1);
  if(e->efd==-1){
    fprintf(stderr,"%s failure! %s\n",__func__,strerror(errno));
    free(e->node_hash);
    free(e);
    return NULL;
  }
  return e;
}

void execute_delete(struct ev_execute * e){
  close(e->efd);
  free(e->node_hash);
  free(e);
}

int execute_add(struct ev_execute *e,struct ev_node* n){
  int found=0;
  struct epoll_event e;
  e.events=n->flag;
  e.data.ptr=n;
  assert(_hash_find(e,v)==NULL);
  if(epoll_ctl(e->efd,EPOLL_CTL_ADD,n->fd,&e)==-1){
    fprintf(stderr,"%s failure! %s\n",__func__,strerror(errno));
    return -1;
  }
  _hash_add(e,n);
  if(e->node_now>=e->node_max*0.75){
    _rehash(e);
  }
  return 0;
}

int execute_mod(struct ev_execute *e,struct ev_node* n){
  int found=0;
  struct epoll_event e;
  e.events=n->flag;
  e.data.ptr=n;
  assert(_hash_find(e,v));
  if(epoll_ctl(e->efd,EPOLL_CTL_MOD,n->fd,&e)==-1){
    fprintf(stderr,"%s failure! %s\n",__func__,strerror(errno));
    return -1;
  }
  return 0;
}

int execute_del(struct ev_execute *e,struct ev_node* n){
  if(!_hash_del(e,n)){
    fprintf(stderr,"%s failure! can not find the node!\n",__func__);
    return -1;
  }

  struct epoll_event e;
  e.events=n->flag;
  e.data.ptr=n;
  if(epoll_ctl(e->efd,EPOLL_CTL_DEL,n->fd,&e)==-1){
    fprintf(stderr,"%s failure! %s\n",__func__,strerror(errno));
  }
  return 0;
}

int execute_runonece(struct ev_execute* e){
  struct epoll_event ev[64],*e;
  struct ev_node* node;
  int ret,i;

  ret=epoll_wait(e->efd,ev,64,-1);
  if(ret>0){
    i=0;
    for(i=0;i<ret;i++){
      e=&ev[i];
      node=e.data.ptr;
      node->call(e,node,e->events);
    }
  }
}
