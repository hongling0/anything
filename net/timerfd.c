#include <stdio.h>
#include "ev.h"

struct socketfd_node{
	struct fd_node fd;
	struct ev_node read;
	struct ev_node write;
};

static void fd_handle(struct ev_execute* e,struct ev_node* n,int events){
	struct socketfd_node* node=(struct socketfd_node*)n;
	struct ev_node* ev;
	if(events&EPOLLOUT){
		ev=&node->write;
		if(ev->active){
			ev->enable=1;
			ev->handle(e,ev);
		}
	}
	if(events&EPOLLIN){
		ev=&node->read;
		if(ev->active){
			ev->enable=1;
			ev->handle(e,ev);
		}
	}
}


int socketfd_read(struct ev_execute* e,struct ev_node* node,unsigned char* buf,size_t sz){
	socklen_t socklen=(ssize_t)sz;
	int n=recv(node->fd,buf,&socklen,0);
	if(n==-1){
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
		return n;
	}
}

int socketfd_write(struct ev_execute* e,struct ev_node* node,unsigned char* buf,size_t sz){
	socklen_t socklen=(ssize_t)sz;
	int n=send(node->fd,buf,&socklen,0);
	if(n==-1){
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
		return n;
	}
}