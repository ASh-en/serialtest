#ifndef __FRM_BUFF_H__
#define __FRM_BUFF_H__

#include "types.h"

typedef struct
{
	U32 pFromBuf;	//待读取的位置
	U32 pToBuf;		//待写入的位置
	U32 bufSize;	//缓冲区长度
    U32 usedSize;   //已使用的缓冲区长度
	U8 *buf;		//缓冲区指针
}SStaticRng;

typedef SStaticRng *SStaticRngId;

// 创建缓冲区
U8 Sr_Create(SStaticRngId pRnd, U8 *pBuf, U32 size);
SStaticRngId Sr_CreateDynamic(U32 size);
void Sr_DestroyDynamic(SStaticRngId rngId);
U32 Sr_NBytes(const SStaticRngId rngId);
S32 Sr_BufPut(SStaticRngId rngId, const U8 *pBuf, S32 nbytes);
S32 Sr_BufGet(SStaticRngId rngId, U8 *pBuf, S32 maxbytes);
S32 Sr_BufGetNoDel(SStaticRngId rngId, U8 *pBuf, S32 maxbytes);
S32 Sr_BufDrop(SStaticRngId rngId, S32 size);
#endif/*__FRM_BUFF_H__*/
