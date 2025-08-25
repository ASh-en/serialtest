#include <stdio.h>
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
	rngId->buf = pBuf;

	return TRUE;
}

U32 Sr_NBytes(const SStaticRngId rngId)
{
	return ((rngId->pToBuf + rngId->bufSize - rngId->pFromBuf) % rngId->bufSize);
}

U32 Sr_BufPut(SStaticRngId rngId, const U8 *pBuf, S32 nbytes)
{
	U32 len, used;

	used = Sr_NBytes(rngId);

	
	if(used + nbytes > rngId->bufSize)
	{
		nbytes = rngId->bufSize - used;
	}

	if(rngId->pToBuf + nbytes < rngId->bufSize)
	{
		memcpy(&rngId->buf[rngId->pToBuf], pBuf, nbytes);
		rngId->pToBuf += nbytes;
	}
	else
	{
		len = rngId->bufSize - rngId->pToBuf;
		memcpy(&rngId->buf[rngId->pToBuf], pBuf, len);
		memcpy(rngId->buf, &pBuf[len], nbytes - len);
		rngId->pToBuf = nbytes - len;
	}

	return nbytes;
}

U32 Sr_BufGet(SStaticRngId rngId, U8 *pBuf, S32 maxbytes)
{
	U32 len, used;

	if((pBuf == NULL) || (maxbytes == 0))
	{
		return FALSE;
	}

	used = Sr_NBytes(rngId);
	if(maxbytes > used)
	{
		maxbytes = used;
	}

	if(rngId->pFromBuf + maxbytes < rngId->bufSize)
	{
		memcpy(pBuf, &rngId->buf[rngId->pFromBuf], maxbytes);
		rngId->pFromBuf += maxbytes;
	}
	else
	{
		len = rngId->bufSize - rngId->pFromBuf;
		memcpy(pBuf, &rngId->buf[rngId->pFromBuf], len);
		memcpy(&pBuf[len], rngId->buf, maxbytes - len);
		rngId->pFromBuf = maxbytes - len;
	}

	return maxbytes;
}

U32 Sr_BufGetNoDel(SStaticRngId rngId, U8 *pBuf, S32 maxbytes)
{
	U32 len, tmp;

	tmp = rngId->pFromBuf;
	len = Sr_BufGet(rngId, pBuf, maxbytes);
	rngId->pFromBuf = tmp;

	return len;
}

U32 Sr_BufDrop(SStaticRngId rngId, U32 size)
{
    if (rngId == NULL)
    {
        return FALSE;
    }

    // 获取缓冲区中当前可用的字节数
    U32 available = Sr_NBytes(rngId);

    // 如果需要丢弃的字节数大于可用字节数，调整为可用字节数
    if (size > available)
    {
        size = available;
    }

    // 计算新的指针位置，防止溢出
    U32 newPFromBuf = rngId->pFromBuf + size;

    // 如果没有回绕，直接移动指针
    if (newPFromBuf < rngId->bufSize)
    {
        rngId->pFromBuf = newPFromBuf;
    }
    else
    {
        // 如果超出了缓冲区大小，回绕到缓冲区的开头
        rngId->pFromBuf = newPFromBuf - rngId->bufSize;
    }

    return size;  // 返回丢弃的字节数
}

