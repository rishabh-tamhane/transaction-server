
#include "protocol.h"
#include "csapp.h"
#include "debug.h"

int proto_send_packet(int fd, XACTO_PACKET *pkt, void *data){

    debug("proto_send_packet");

    debug("Packet To Be Sent - Type:%u\tStatus:%u\tNull:%u\tSerial:%u\tSize:%u\tTimestamp_sec:%u\tTimestamp_nsec:%u\n",pkt->type,pkt->status,pkt->null,ntohl(pkt->serial),ntohl(pkt->size),ntohl(pkt->timestamp_sec),ntohl(pkt->timestamp_nsec));

    Rio_writen(fd,(void*)pkt,sizeof(XACTO_PACKET));

    debug("rio_written header");

    if(ntohl(pkt->size)>0){
        Rio_writen(fd,data,ntohl(pkt->size));
        debug("rio_written data");
    }
    /*
    else{
        debug("no data");
        debug("dump");
        Rio_writen(fd,data,ntohl(pkt->size));
        debug("did not dump");
    }
    */

    return 0;
}


int proto_recv_packet(int fd, XACTO_PACKET *pkt, void **datap){

    if(fd==-1 || pkt==NULL)
        return -1;

    /*
    if((pkt->type)==0)
        return 0;
    */
    if(rio_readn(fd,(void *)pkt,sizeof(XACTO_PACKET))<=0){
        debug("packet_read_error");
        return -1;
    }

    debug("Packet Received - Type:%u\tStatus:%u\tNull:%u\tSerial:%u\tSize:%u\tTimestamp_sec:%u\tTimestamp_nsec:%u\n",(pkt->type),(pkt->status),ntohl(pkt->null),ntohl(pkt->serial),ntohl(pkt->size),ntohl(pkt->timestamp_sec),ntohl(pkt->timestamp_nsec));
    
    int data_size = ntohl(pkt->size);

    if(data_size>0){

        *datap = Malloc(data_size);
        debug("malloced :%d",data_size);

        if(rio_readn(fd,*datap,data_size)<=0){
            debug("data_read_error");
            return -1;
        }
    }


    return 0;
}

