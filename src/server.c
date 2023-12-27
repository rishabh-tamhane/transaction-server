
#include "server.h"
#include "csapp.h"
#include "protocol.h"
#include "transaction.h"
#include "debug.h"
#include "data.h"
#include "store.h"
#include <time.h>

CLIENT_REGISTRY *client_registry;

void *xacto_client_service(void *arg){

    int fd=*((int*)arg);
    debug("[%d] Starting Client Service",fd);
    free(arg);

    Pthread_detach(pthread_self());
    debug("Thread Detached");

    creg_register(client_registry,fd);
    debug("Client Registered");

    TRANSACTION *txn= trans_create();
    debug("Create New Transaction %u",txn->id);


    //keeping an empty packet and keep passing it to the functions else malloc and free would be the plan
    XACTO_PACKET *pkt = malloc(sizeof(XACTO_PACKET));
    memset(pkt,0,sizeof(XACTO_PACKET));


    void *ptr=NULL;

    while(proto_recv_packet(fd,pkt,&ptr)==0){

        if(pkt->type == XACTO_PUT_PKT){

            debug("PUT PACKET RECEIVED");

            void *key_data_ptr;
            size_t key_data_size;

            if(proto_recv_packet(fd,pkt,&ptr)<0){
                break;  
            }       
            if(pkt->type == XACTO_KEY_PKT){
                debug("KEY PACKET RECEIVED: Size %u",ntohl(pkt->size));
                key_data_ptr = ptr;
                key_data_size=ntohl(pkt->size);
            }
            else 
                break;


            void *value_data_ptr;
            size_t value_data_size;
  
            if(proto_recv_packet(fd,pkt,&ptr)<0)
                break; 
            if(pkt->type == XACTO_VALUE_PKT){
                debug("VALUE PACKET RECEIVED: Size %u",ntohl(pkt->size));
                value_data_ptr = ptr;
                value_data_size=ntohl(pkt->size);
            }
            else
                break;

            BLOB *key_blob = blob_create((char*)key_data_ptr,key_data_size);
            debug("BLOB SIZE VALUE: %zu",key_blob->size);

            KEY *key = key_create(key_blob);
            debug("KEY for Blob %p",key->blob);
            free(key_data_ptr);

            BLOB *value_blob=blob_create((char*)value_data_ptr,value_data_size);
            if(value_blob==NULL)
                break;
            debug("BLOB SIZE VALUE: %zu",value_blob->size);

            free(value_data_ptr);
            TRANS_STATUS txn_status= store_put(txn,key,value_blob);
            debug("txn status updated");

            if(txn_status==TRANS_ABORTED){
                debug("Received trans aborted from store put");
                break;
            }
            else if(txn_status==TRANS_PENDING){
                debug("Building the send packet");
                struct timespec time;
                clock_gettime(CLOCK_MONOTONIC,&time);
                pkt->type=XACTO_REPLY_PKT;
                pkt->size=0;
                pkt->timestamp_sec=htonl(time.tv_sec);
                pkt->timestamp_nsec=htonl(time.tv_nsec);

                if(proto_send_packet(fd,pkt,NULL)==0)
                    debug("Sent Reply Packet");
            }
        }
     
        else if(pkt->type == XACTO_GET_PKT){
                debug("GET PACKET RECEIVED");

                void **key_data_ptr;
                size_t key_data_size;

                if(proto_recv_packet(fd,pkt,&ptr)<0){
                    debug("proto recv error");
                    break;   
                }      
                if(pkt->type == XACTO_KEY_PKT){
                    debug("KEY PACKET RECEIVED: Size %u",ntohl(pkt->size));
                    key_data_ptr = ptr;
                    key_data_size=ntohl(pkt->size);
                }
                else 
                    break;


                BLOB *key_blob = blob_create((char*)key_data_ptr,key_data_size);
                debug("BLOB SIZE VALUE: %zu",key_blob->size);

                KEY *key = key_create(key_blob);
                debug("KEY for Blob %p",key->blob);
                free(key_data_ptr);
                TRANS_STATUS txn_status= store_get(txn,key,&key_blob);
                debug("txn status updated %d",txn_status);


                if(txn_status==TRANS_ABORTED){
                    debug("Received trans aborted from store put");
                    break;
                    
                }
                else if(txn_status==TRANS_PENDING){
                    debug("Building the send packet");
                    struct timespec time;
                    clock_gettime(CLOCK_MONOTONIC,&time);
                    pkt->type=XACTO_REPLY_PKT;
                    pkt->size=0;
                    pkt->timestamp_sec=htonl(time.tv_sec);
                    pkt->timestamp_nsec=htonl(time.tv_nsec);

                    if(proto_send_packet(fd,pkt,NULL)==0)
                        debug("Sent Reply Packet");
                    if(key_blob!=NULL){
                        debug("Value : %s",key_blob->prefix);
                        blob_unref(key_blob,"for returning from store get");

                        clock_gettime(CLOCK_MONOTONIC,&time);
                        pkt->type=XACTO_VALUE_PKT;
                        pkt->size=htonl(key_blob->size);
                        pkt->timestamp_sec=htonl(time.tv_sec);
                        pkt->timestamp_nsec=htonl(time.tv_nsec);

                        if(proto_send_packet(fd,pkt,key_blob->content)==0)
                            debug("Sent Value Packet");
                    }
                    else{
                        debug("Value : NULL");

                        clock_gettime(CLOCK_MONOTONIC,&time);
                        pkt->type=XACTO_VALUE_PKT;
                        pkt->size=0;
                        pkt->null=1;
                        pkt->timestamp_sec=htonl(time.tv_sec);
                        pkt->timestamp_nsec=htonl(time.tv_nsec);

                        if(proto_send_packet(fd,pkt,NULL)==0)
                            debug("Sent Reply Packet");


                    }



                        
            }   


        }

    
        else if(pkt->type == XACTO_COMMIT_PKT){
                debug("[%d] COMMIT PACKET RECEIVED",fd);
       

                struct timespec time;
                clock_gettime(CLOCK_MONOTONIC,&time);
                pkt->type=XACTO_REPLY_PKT;
                pkt->size=0;
                pkt->status=trans_commit(txn);
                pkt->timestamp_sec=htonl(time.tv_sec);
                pkt->timestamp_nsec=htonl(time.tv_nsec);

                if(proto_send_packet(fd,pkt,NULL)==0)
                {
                    debug("Sent Reply Packet");
                }

                creg_unregister(client_registry,fd);
                free(pkt);
                Close(fd);
                return NULL;


        }

    }

    debug("Out of service loop");
    trans_abort(txn);
    free(pkt);
    creg_unregister(client_registry,fd);

    Close(fd);
    return NULL;


}
