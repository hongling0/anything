#ifndef LIB__POLLER_H
#define LIB__POLLER_H
#include <stint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ev_execute;
struct ev_node;

typedef void(*ev_call)(struct ev_execute* e,struct ev_node* ev,node);
typedef void(*ev_free)(struct ev_node* ev,node);

struct ev_node{
  struct ev_node* next;
  uint32_t id;
  uint32_t obj;
  int fd;
  int flag;
  ev_call call;
  ev_free free;
};

struct ev_node* ev_node_create(size_t sz,ev_free free);
struct void ev_node_delete(struct ev_node* n);

struct ev_execute* execute_create(void);
void execute_delete(struct ev_execute *e);

struct ev_node* execute_get(struct ev_execute *e,uint32_t id);

int execute_add(struct ev_execute *e,struct ev_node* n);
int execute_mod(struct ev_execute *e,struct ev_node* n);
int execute_del(struct ev_execute *e,struct ev_node* n);
int execute_runonece(struct ev_execute* e);



#ifdef __cplusplus
}
#endif

#endif //LIB__POLLER_H