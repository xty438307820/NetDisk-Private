#include "work_que.h"

int queInsert(pQue pq,pNode pnode){
    if(pq->currentSize==pq->Capacity){
        return -1;
    }
    if(NULL==pq->phead){
        pq->phead=pnode;
        pq->pTail=pnode;
    }
    else{
        pq->pTail->nextNode=pnode;
        pq->pTail=pnode;
    }
    pq->currentSize++;
    return 0;
}

int quePop(pQue pq,pNode* pnode){
    if(pq->currentSize==0){
        return -1;
    }
    (*pnode)=pq->phead;
    pq->phead=pq->phead->nextNode;
    pq->currentSize--;
    return 0;
}

int queEmpty(pQue pq){
    if(pq->currentSize==0) return 1;
    return 0;
}

