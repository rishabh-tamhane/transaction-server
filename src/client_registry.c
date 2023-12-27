
#include "csapp.h"
#include "client_registry.h"
#include "debug.h"

// Keeping track of the file descriptors for clients that are currently connected
struct client_registry{

    int fd_conn[FD_SETSIZE];          //file descriptor pointer
    int client_count;                   //client count
    sem_t mutex;                          //mutex to protect fd_conn  
    sem_t zero_flag;                     //zero clients : 1 if it is zero
};

CLIENT_REGISTRY *creg_init(){

    CLIENT_REGISTRY *client_reg = Malloc(sizeof(CLIENT_REGISTRY));
    debug("Initializing Client Registry: Begin");
    for(int i=0;i<FD_SETSIZE;i++){
        client_reg->fd_conn[i]=0;
    }

    client_reg->client_count=0;
    Sem_init(&client_reg->mutex,0,1);
    Sem_init(&client_reg->zero_flag,0,1);

    debug("Initializing Client Registry: End");
    return client_reg;
}

void creg_fini(CLIENT_REGISTRY *cr){

    debug("Closing File Registry and freeing the resources");
    free(cr);
}

int creg_register(CLIENT_REGISTRY *cr, int fd){

    debug("Registration of Client: Begin");
    P(&cr->mutex);              //lock the registry

    if(cr->fd_conn[fd]==0)
        cr->fd_conn[fd]=1;
    else 
        return -1;

    (cr->client_count)=(cr->client_count)+1;

    if((cr->client_count)==1)
        P(&cr->zero_flag);

    debug("Registered client fd %d (Total Clients=%d)",fd,cr->client_count);
    V(&cr->mutex);              //unlock the registry

    debug("Registration of Client: End");
    return 0;
}

int creg_unregister(CLIENT_REGISTRY *cr, int fd){

    debug("Unregister Client: Begin");
    P(&cr->mutex);              //lock the registry

    if(cr->fd_conn[fd]==1)
        cr->fd_conn[fd]=0;
    else 
        return -1;

    (cr->client_count)=(cr->client_count)-1;

    if((cr->client_count)==0)
        V(&cr->zero_flag);

    debug("Unregistered client fd %d (Total Clients=%d)",fd,cr->client_count);
    V(&cr->mutex);          //unlock the registry

    debug("Unregister Client: End");
    return 0;
}

void creg_wait_for_empty(CLIENT_REGISTRY *cr){

    debug("Waiting for the registered client count to go to 0");
    P(&cr->zero_flag);
}

void creg_shutdown_all(CLIENT_REGISTRY *cr){

    P(&cr->mutex);

    for(int i=3;i<FD_SETSIZE;i++){
        if(cr->fd_conn[i]==1){
            debug("Shutting down file descriptor: %d",i);
            shutdown(i,SHUT_RD);
        }
    }

    V(&cr->mutex);
}
