#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "FrmBuf.h"


U8 Sr_Create(SStaticRngId rngId, U8 *pBuf, U32 size)
{
	if(rngId == NULL || pBuf == NULL)
	{
		return FALSE;
	}

	rngId->pFromBuf = 0;
	rngId->pToBuf = 0;
	rngId->bufSize = size;
    rngId->usedSize = 0;
	rngId->buf = pBuf;

	return TRUE;
}

SStaticRngId Sr_CreateDynamic(U32 size)
{
    // 一次性申请：结构体 + 缓冲区
    SStaticRngId rngId = (SStaticRngId)malloc(sizeof(SStaticRng) + size);
    if (!rngId)
    {
        return NULL;
    }
    rngId->pFromBuf = 0;
    rngId->pToBuf   = 0;
    rngId->bufSize  = size;
    rngId->usedSize = 0;

    // 缓冲区紧跟在结构体后面
    rngId->buf = (U8*)(rngId + 1);

    return rngId;
}

void Sr_DestroyDynamic(SStaticRngId rngId)
{
    if (rngId != NULL)
    {
        free(rngId);
    }
        
}

U32 Sr_NBytes(const SStaticRngId rngId)
{
    return rngId->usedSize;
	//return ((rngId->pToBuf + rngId->bufSize - rngId->pFromBuf) % rngId->bufSize);
}

S32 Sr_BufPut(SStaticRngId rngId, const U8 *pBuf, S32 nbytes)
{
    if (rngId == NULL || pBuf == NULL || nbytes < 0)
    {
        return FALSE;
    }

	S32 used, free, tail;

	used = rngId->usedSize;
    free = rngId->bufSize - used;
    nbytes = (nbytes  >  free) ? free : nbytes;

	tail = rngId->bufSize - rngId->pToBuf;

	if(nbytes <= tail)
	{
		memcpy(&rngId->buf[rngId->pToBuf], pBuf, nbytes);
		rngId->pToBuf += nbytes;
	}
	else
	{
	
		memcpy(&rngId->buf[rngId->pToBuf], pBuf, tail);
		memcpy(rngId->buf, &pBuf[tail], nbytes - tail);
		rngId->pToBuf = nbytes - tail;
	}
    rngId->usedSize += nbytes;

	return nbytes;
}

S32 Sr_BufGet(SStaticRngId rngId, U8 *pBuf, S32 maxbytes)
{
    if (rngId == NULL || pBuf == NULL || maxbytes < 0)
    {
        return FALSE;
    }
    
	U32 used, tail;
	
	used = rngId->usedSize;
    maxbytes = (maxbytes > used) ? used : maxbytes;
    tail = rngId->bufSize - rngId->pFromBuf;

	if(maxbytes <= tail)
	{
		memcpy(pBuf, &rngId->buf[rngId->pFromBuf], maxbytes);
		rngId->pFromBuf += maxbytes;
	}
	else
	{
		memcpy(pBuf, &rngId->buf[rngId->pFromBuf], tail);
		memcpy(&pBuf[tail], rngId->buf, maxbytes - tail);
		rngId->pFromBuf = maxbytes - tail;
	}
    rngId->usedSize -= maxbytes;
	return maxbytes;
}

S32 Sr_BufGetNoDel(SStaticRngId rngId, U8 *pBuf, S32 maxbytes)
{
    if (rngId == NULL || pBuf == NULL || maxbytes < 0)
    {
        return FALSE;
    }
	S32 len, tmp;
	tmp = rngId->pFromBuf;
	len = Sr_BufGet(rngId, pBuf, maxbytes);
    rngId->usedSize += len;
	rngId->pFromBuf = tmp;
	return len;
}

S32 Sr_BufDrop(SStaticRngId rngId, S32 size)
{
    if (rngId == NULL)
    {
        return FALSE;
    }
    
    U32 used = rngId->usedSize;         // 获取缓冲区中当前可用的字节数
    size = (size > used) ? used : size; // 如果请求丢弃的字节数大于可用字节数，则调整为可用字节数
    if (size == 0)
    {
        return 0;
    }
    // 计算新的指针位置，防止溢出
    rngId->pFromBuf = (rngId->pFromBuf + size) % rngId->bufSize;
    rngId->usedSize -= size;

    return size;  // 返回丢弃的字节数
}

