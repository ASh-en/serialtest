#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "FrmBuf.h"


S8 Sr_Create(SStaticRngId rngId, U8 *pBuf, U32 size)
{
	if(rngId == NULL || pBuf == NULL || size == 0)
	{
		return FALSE;
	}

	rngId->pFromBuf = 0;
	rngId->pToBuf = 0;
	rngId->bufSize = size;
    rngId->usedSize = 0;
    InitializeCriticalSection(&rngId->lock); 
	rngId->buf = pBuf;

	return TRUE;
}

void Sr_Destroy(SStaticRngId rngId)
{
    if (rngId == NULL)
    {
        return ;
    }
    rngId->pFromBuf = 0;
	rngId->pToBuf = 0;
	rngId->bufSize = 0;
    rngId->usedSize = 0; 
    DeleteCriticalSection(&rngId->lock); 
    rngId->buf = NULL;
    return ;
        
}

SStaticRngId Sr_CreateDynamic(U32 size)
{
    if (size == 0)
    {
        return NULL;
    }
    // 一次性申请：结构体 + 缓冲区
    SStaticRngId rngId = (SStaticRngId)malloc(sizeof(SStaticRng) + size);
    if (!rngId)
    {
        return NULL;
    }
    rngId->pFromBuf = 0;
    rngId->pToBuf   = 0;
    rngId->bufSize  = size;
    InitializeCriticalSection(&rngId->lock); 
    rngId->usedSize = 0;

    // 缓冲区紧跟在结构体后面
    rngId->buf = (U8*)(rngId + 1);

    return rngId;
}



void Sr_DestroyDynamic(SStaticRngId rngId)
{
    if (rngId != NULL)
    {
        DeleteCriticalSection(&rngId->lock); 
        free(rngId);
    }
        
}

U32 Sr_NBytes(const SStaticRngId rngId)
{
    EnterCriticalSection(&rngId->lock);
    U32 used = rngId->usedSize;
    LeaveCriticalSection(&rngId->lock);
    return used;
	//return ((rngId->pToBuf + rngId->bufSize - rngId->pFromBuf) % rngId->bufSize);
}

S32 Sr_BufPut(SStaticRngId rngId, const U8 *pBuf, U32 putbytes)
{
    if (rngId == NULL || pBuf == NULL || putbytes == 0)
    {
        return -1;
    }
    EnterCriticalSection(&rngId->lock); // 加锁
	U32 free, tail;

    free = rngId->bufSize - rngId->usedSize;
    putbytes = (putbytes  >  free) ? free : putbytes;

	tail = rngId->bufSize - rngId->pToBuf;

	if(putbytes <= tail)
	{
		memcpy(&rngId->buf[rngId->pToBuf], pBuf, putbytes);
		rngId->pToBuf = (rngId->pToBuf + putbytes) % rngId->bufSize;
        
	}
	else
	{
	
		memcpy(&rngId->buf[rngId->pToBuf], pBuf, tail);
		memcpy(rngId->buf, &pBuf[tail], putbytes - tail);
		rngId->pToBuf = (putbytes - tail) % rngId->bufSize;
	}
    rngId->usedSize += putbytes;
    if(rngId->usedSize > rngId->bufSize)
    {
        printf("Sr_BufPut error: usedSize overflow!\n");
        rngId->usedSize = rngId->bufSize; // 防止溢出
    }
    LeaveCriticalSection(&rngId->lock); // 解锁
	return (S32)putbytes;
}

S32 Sr_BufGet(SStaticRngId rngId, U8 *pBuf, U32 getbytes)
{
    if (rngId == NULL || pBuf == NULL || getbytes == 0)
    {
        return -1;
    }
    EnterCriticalSection(&rngId->lock); // 加锁
	U32 used, tail;
	
	used = rngId->usedSize;
    getbytes = (getbytes > used) ? used : getbytes;
    tail = rngId->bufSize - rngId->pFromBuf;

	if(getbytes <= tail)
	{
		memcpy(pBuf, &rngId->buf[rngId->pFromBuf], getbytes);
		rngId->pFromBuf = (rngId->pFromBuf + getbytes) % rngId->bufSize;

	}
	else
	{
		memcpy(pBuf, &rngId->buf[rngId->pFromBuf], tail);
		memcpy(&pBuf[tail], rngId->buf, getbytes - tail);
		rngId->pFromBuf = (getbytes - tail) % rngId->bufSize;
	}
    rngId->usedSize -= getbytes;
    if(rngId->usedSize < 0)
    {
        printf("Sr_BufGet error: usedSize underflow!\n");
        rngId->usedSize = 0; // 防止溢出
    }
    LeaveCriticalSection(&rngId->lock); // 解锁
	return (S32)getbytes;
}

S32 Sr_BufGetNoDel(SStaticRngId rngId, U8 *pBuf, U32 getbytes)
{
    if (rngId == NULL || pBuf == NULL || getbytes == 0)
    {
        return -1;
    }
    EnterCriticalSection(&rngId->lock); 
	U32 used, tail;

    used = rngId->usedSize;
    getbytes = (getbytes > used) ? used : getbytes;
    tail = rngId->bufSize - rngId->pFromBuf;

	if(getbytes <= tail)
	{
		memcpy(pBuf, &rngId->buf[rngId->pFromBuf], getbytes);
	}
	else
	{
		memcpy(pBuf, &rngId->buf[rngId->pFromBuf], tail);
		memcpy(&pBuf[tail], rngId->buf, getbytes - tail);
	}
    
    LeaveCriticalSection(&rngId->lock); // 解锁
	return (S32)getbytes;
}

S32 Sr_BufDrop(SStaticRngId rngId, U32 dropbytes)
{
    if (rngId == NULL || dropbytes == 0)
    {
        return -1;
    }
    EnterCriticalSection(&rngId->lock); // 加锁
    U32 used = rngId->usedSize;         // 获取缓冲区中当前可用的字节数
    dropbytes = (dropbytes > used) ? used : dropbytes; // 如果请求丢弃的字节数大于可用字节数，则调整为可用字节数
    
    // 计算新的指针位置，防止溢出
    rngId->pFromBuf = (rngId->pFromBuf + dropbytes) % rngId->bufSize;
    rngId->usedSize -= dropbytes;
    if(rngId->usedSize < 0)
    {
        printf("Sr_BufDrop error: usedSize underflow!\n");
        rngId->usedSize = 0; // 防止溢出
    }
    LeaveCriticalSection(&rngId->lock); // 解锁
    return dropbytes;  // 返回丢弃的字节数
}

