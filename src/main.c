#include "debug.h"
#include "client_registry.h"
#include "transaction.h"
#include "store.h"
#include "csapp.h"
#include "server.h"

static void terminate(int status);
void sighup_handler(int sig_val);

CLIENT_REGISTRY *client_registry;

int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    if(argc!=3){
        fprintf(stderr,"usage: bin/xacto -p <port>\n");
        exit(0);
    }
    else if(argc==3 && strcmp(argv[1],"-p")!=0){
        fprintf(stderr,"usage: bin/xacto -p <port>\n");
        exit(0);
    }
    
    // Perform required initializations of the client_registry,
    // transaction manager, and object store.
    client_registry = creg_init();
    trans_init();
    store_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function xacto_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.
    debug("Installing SIGHUP Handler");
    Signal(SIGHUP,(handler_t *)&sighup_handler);

    int *connfdp;
    int listenfd;
    socklen_t client_len;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    listenfd = Open_listenfd(argv[2]);

    while(1){
        client_len=sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int));
        *connfdp = accept(listenfd,(SA*) &clientaddr, &client_len);
        debug("Connection accepted on %d",*connfdp);
        if(*connfdp<0){
            free(connfdp);
            terminate(EXIT_FAILURE);
        }
        Pthread_create(&tid,NULL,(void*)xacto_client_service,connfdp);
    }
    fprintf(stderr, "You have to finish implementing main() "
	    "before the Xacto server will function.\n");
    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);
    
    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");

    // Finalize modules.
    creg_fini(client_registry);
    trans_fini();
    store_fini();

    debug("Xacto server terminating");
    exit(status);
}

/*
SIGHUP Handler to perform a clean shutdown of the program 
*/
void sighup_handler(int sig_val){
    terminate(EXIT_SUCCESS);
}
