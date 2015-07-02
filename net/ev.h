#ifndef LIB__POLLER_H
#define LIB__POLLER_H
#include <stint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ev_execute;
struct ev_node;
struct fd_node;

typedef void(*ev_handle)(struct ev_execute* e,struct ev_node* n);
typedef void(*fd_handle)(struct ev_execute* e,struct ev_node* n,int events);

struct ev_node{
	unsigned active:1;
	ev_handle handle;
	void* ctx;
};

struct fd_node{
	int fd;
	int events;
	fd_handle handle;
};

struct socketfd_node{
	struct fd_node fd;
	struct ev_node read;
	struct ev_node write;
};

struct listen_node{
	struct fd_node fd;
	struct ev_node read;
};

struct timerfd_node{
	struct fd_node fd;
	struct ev_node read;
};

struct eventfd_node{
	struct fd_node fd;
	struct ev_node read;
	struct ev_node write;
};

struct fifofd_node{
	struct fd_node fd;
	struct ev_node read;
	struct ev_node write;
};

struct ev_execute{
	int efd;
};

struct ev_execute* execute_create(void);
void execute_delete(struct ev_execute *e);


int execute_add(struct ev_execute *e,struct fd_node* n);

int execute_mod(struct ev_execute *e,struct fd_node* n);
int execute_del(struct ev_execute *e,struct fd_node* n);

int execute_runonece(struct ev_execute* e);


#ifdef __cplusplus
}
#endif

#endif //LIB__POLLER_H