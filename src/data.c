
#include "data.h"
#include "debug.h"
#include "csapp.h"
#include "transaction.h"


BLOB *blob_create(char *content, size_t size){


    BLOB *blob_ptr = malloc(sizeof(BLOB));
    debug("Create blob with content %p, size %zu -> %p",content,size,blob_ptr);
    pthread_mutex_init(&blob_ptr->mutex,NULL);
    blob_ptr->refcnt=1;
    blob_ptr->size=size;
    blob_ptr->content=malloc(size);
    memcpy(blob_ptr->content,content,size);
    blob_ptr->prefix=malloc(size+1);
    memcpy(blob_ptr->prefix,content,size);
    blob_ptr->prefix[size]=0;
    debug("Increase reference count on blob %p  (0->%d) for newly created blob",blob_ptr,blob_ptr->refcnt);
    // how to add prefix?

    return blob_ptr;

}

BLOB *blob_ref(BLOB *bp, char *why){

    pthread_mutex_lock(&bp->mutex);

    bp->refcnt=(bp->refcnt)+1;
    debug("Increase Reference Count on blob %p[%s] (%d -> %d) : %s",bp,bp->prefix,bp->refcnt-1,bp->refcnt,why);

    pthread_mutex_unlock(&bp->mutex);

    return bp;
}


void blob_unref(BLOB *bp, char *why){

    pthread_mutex_lock(&bp->mutex);

    bp->refcnt=(bp->refcnt)-1;
    debug("Decrease Reference Count on blob %p[%s] (%d -> %d) : %s",bp,bp->prefix,bp->refcnt+1,bp->refcnt,why);

    if(bp->refcnt==0){
        debug("Free Blob %p",bp);
        pthread_mutex_unlock(&bp->mutex);
        free(bp->content);
        free(bp->prefix);
        free(bp);

    }

    else 
        pthread_mutex_unlock(&bp->mutex);

}


int blob_compare(BLOB *bp1, BLOB *bp2){

    pthread_mutex_lock(&bp1->mutex);
    pthread_mutex_lock(&bp2->mutex);
    debug("Blob Compare");
    size_t cmp_size;
    int diff_flag=0;
    if(bp1->size!=bp2->size)
        diff_flag=1;
    else 
        cmp_size=bp1->size;

    if(memcmp(bp1->content,bp2->content,cmp_size)==0 && diff_flag==0){
        pthread_mutex_unlock(&bp2->mutex);
        pthread_mutex_unlock(&bp1->mutex);
        return 0;
    }
    else{
        pthread_mutex_unlock(&bp2->mutex);
        pthread_mutex_unlock(&bp1->mutex);
        return -1;
    }
}

int blob_hash(BLOB *bp){

    debug("Blob Hash");
    int hash=0;

    //MAKE YOUR OWN HASH FUNCTION later
    char *str=bp->content;
    for(size_t i=0;i<bp->size;i++){
        hash=(hash*31)+(int)str[i];
    }

    debug("Blob Hash End");
    return hash;
}


KEY *key_create(BLOB *bp){

    debug("Key Create");
    KEY *key_ptr=Malloc(sizeof(KEY));

    pthread_mutex_lock(&bp->mutex);
    
    key_ptr->hash=blob_hash(bp);
    key_ptr->blob=bp;
    //bp->refcnt=(bp->refcnt)+1; 
    debug("Create key from blob %p -> %p ",bp,key_ptr);
    pthread_mutex_unlock(&bp->mutex);

    return key_ptr;
}

void key_dispose(KEY *kp){



    debug("Key Dispose");
    BLOB *bp=kp->blob;

    blob_unref(bp,"key dispose");
    free(kp);


}

int key_compare(KEY *kp1, KEY *kp2){

    //blob compare
    debug("Key Compare");
    return blob_compare(kp1->blob,kp2->blob);
}

VERSION *version_create(TRANSACTION *tp, BLOB *bp){

    debug("Version Create");
    VERSION *version_ptr = Malloc(sizeof(VERSION));

    version_ptr->creator=tp;
    version_ptr->blob=bp;
    version_ptr->next=NULL;
    version_ptr->prev=NULL;

    trans_ref(tp,"version_create");

    debug("Version Create End");
    return version_ptr;

}

void version_dispose(VERSION *vp){

    debug("Version Dispose: %p",vp->blob);
    if(vp->blob!=NULL){
    debug("Blocked here");
    trans_unref((vp->creator),"version dispose transaction creator unref");
    blob_unref((vp->blob),"version dispose blob unref");

    debug("Reached here in Version Dispose");
    //dispose off the reference count of creator transaction
    free(vp);
    }
    else{

        debug("Version Dispose: Blob is NULL");
        trans_unref((vp->creator),"version dispose transaction creator unref");
        free(vp);
    }
    debug("Version Dispose Done");
}
