#include <stdio.h>
#include "CmdFrm.h"
#include "ProcCmd.h"
#include "AsyncFrame.h"



//命令接收过程

//接收缓存区考虑使用中断接收
static SStaticRng mRecvRngId;
static U8 m_FrmRcvBuf[MAX_RB_LEN];

#ifdef SEND_RING_BUF
//发送缓冲区
static SStaticRng mSendRngId;
static U8 m_FrmSndBuf[MAX_RB_LEN];

#endif

static U16 nFrmShortCnt = 0;
static U8 m_pReadBuf[MAX_CMD_LEN];

extern U32 frameCount ;


//全局环形缓存区 mRecvRngId 初始化
void GloabalRingBufInit(void)
{
	Sr_Create(&mRecvRngId, m_FrmRcvBuf, sizeof(m_FrmRcvBuf));

    #ifdef SEND_RING_BUF
    Sr_Create(&mSendRngId, m_FrmSndBuf, sizeof(m_FrmSndBuf));
    #endif

	nFrmShortCnt = 0;
}

S32 PutPrmFrame(const U8 *buf, S32 dataByte)
{
	return Sr_BufPut(&mRecvRngId, buf, dataByte);
}

/**
 * 检查并提取完整帧
 * 返回值：完整帧长度，0表示无完整帧
 * buffer：用于存放提取的完整帧数据
 * 
 * 该函数会检查环形缓冲区中是否存在完整的命令帧，
 * 如果存在，则将其提取到提供的 buffer 中，并返回帧长度。
 * 如果不存在完整帧，则返回 0。
 * 
 * 完整帧格式：
 * 帧头(1字节) + 设备号(1字节) + 长度(2字节) + 数据(n字节) + 校验码(1字节) + 帧尾(1字节)
 * 
 * 注意：
 * - 该函数假设 buffer 足够大以容纳最大可能的帧。
 * - 如果检测到不合法的帧头或长度，将逐字节丢弃数据以寻找下一个可能的帧头。
 * - 对于长时间未接收完的帧，函数会在多次检查后丢弃不完整的数据以防止阻塞。
static U32 FBufferHasExFrame(SStaticRngId rngId, U8 *buffer)
{
	S32 i, len;
	U8 checksum;

	while(Sr_BufGetNoDel(rngId, buffer, uFRAME_MIN_LEN) == uFRAME_MIN_LEN)//有最短帧长可接收
	{
		len = (U16)((buffer[uFRAME_LEN_H_IDX] << 8) | buffer[uFRAME_LEN_L_IDX]) + uFRAME_HE_ND_LEN;//完整帧长
		//帧头正确且长度合法

		if((buffer[0] == FRAME_HEAD)
			&& (len >= uFRAME_MIN_LEN) && (len <= uFRAME_MAX_LEN))
		{
			//数据未接收完
			if(Sr_BufGetNoDel(rngId, buffer, len) != len)
			{
                //如果一个完整帧在2500次检查内未接收完，将导致丢帧，根据实际情况可调整
                //对于长数据帧，该次数应增大。
				if(++nFrmShortCnt < 2500)
				{
					break;
				}
                else 
                {
                    printf("Frame receive timeout, dropping incomplete frame.\n");
                }	
			}
			else
			{
				//计算校验值
				checksum = 0;
				for(i = 0; i < len - uFRAME_END_LEN; ++i)
				{
					checksum ^= buffer[i];
				}

				//整帧完整
				if((buffer[len - 2] == checksum) && (buffer[len - 1] == FRAME_END))
				{
                    
                    frameCount++;
		
					Sr_BufDrop(rngId, uFRAME_HEAD_LEN);//丢弃帧头
					Sr_BufGet(rngId, buffer, len-uFRAME_HE_ND_LEN);//正式取数
					Sr_BufDrop(rngId, uFRAME_END_LEN);//丢弃帧尾
                    //Frm2Cmd(buffer,Cmd,len-6);

					nFrmShortCnt = 0;
					return len;
				}
			}
		}
		Sr_BufDrop(rngId, 1);//逐字节丢弃
		nFrmShortCnt = 0;
	}

	return 0;
}

*/


static U32 FBufferHasExFrame(SStaticRngId rngId, U8 *buffer)
{
    static FrameBufState state = FB_STATE_FIND_HEAD;
    static S32 expectedLen = 0; // 当前帧的期望长度
    U32 available = Sr_NBytes(rngId);

    if (available < uFRAME_MIN_LEN)
    {
        return 0; // 可用数据不足以构成最小帧
    }

    while (1) // 循环驱动状态迁移
    {
        switch (state)
        {
        case FB_STATE_FIND_HEAD:
            while (available >= 1)
            {
                Sr_BufGetNoDel(rngId, buffer, 1);
                if (buffer[0] == FRAME_HEAD)
                {
                    state = FB_STATE_WAIT_LEN; // 状态切换
                    break;                     // 跳出 switch，重新进入 while
                }
                else
                {
                    Sr_BufDrop(rngId, 1); // 丢掉垃圾字节
                    available--;
                }
            }

            // 如果没有足够数据，就退出
            if (state != FB_STATE_WAIT_LEN)
                return 0;
            break;

        case FB_STATE_WAIT_LEN:
            if (available >= uFRAME_MIN_LEN)
            {
                Sr_BufGetNoDel(rngId, buffer, uFRAME_MIN_LEN);
                expectedLen = ((buffer[uFRAME_LEN_H_IDX] << 8) |
                               buffer[uFRAME_LEN_L_IDX]) + uFRAME_HE_ND_LEN;

                if (expectedLen < uFRAME_MIN_LEN || expectedLen > uFRAME_MAX_LEN)
                {
                    // 长度非法，回到找帧头
                    Sr_BufDrop(rngId, 1);
                    expectedLen = 0;
                    available--;
                    state = FB_STATE_FIND_HEAD;
                    break; // 回到 while，再进 FIND_HEAD
                }
                else
                {
                    state = FB_STATE_WAIT_AND_CHECK_FRAME;
                    break; // 回到 while，再进 WAIT_AND_CHECK_FRAME
                }
            }
            return 0; // 数据不够，退出函数

        case FB_STATE_WAIT_AND_CHECK_FRAME:
            if (available >= (U32)expectedLen)
            {
                // 可以取出完整帧
                Sr_BufGetNoDel(rngId, buffer, expectedLen);
                if (buffer[expectedLen - 1] != FRAME_END)
                {
                    // 帧尾错误，丢掉一个字节重新找帧头
                    Sr_BufDrop(rngId, 1);
                    expectedLen = 0;
                    available--;
                    state = FB_STATE_FIND_HEAD;
                    break;
                }
                else
                {
                    // 校验
                    U8 checksum = 0;
                    for (S32 i = 0; i < expectedLen - uFRAME_END_LEN; ++i)
                    {
                        checksum ^= buffer[i];
                    }
                    if (buffer[expectedLen - 2] == checksum)
                    {
                        // 提取负载
                        Sr_BufDrop(rngId, uFRAME_HEAD_LEN);
                        Sr_BufGet(rngId, buffer, expectedLen - uFRAME_HE_ND_LEN);
                        Sr_BufDrop(rngId, uFRAME_END_LEN);

                        frameCount++;
                        state = FB_STATE_FIND_HEAD;
                        available -= expectedLen;
                        // 返回完整帧长度
                        S32 resultLen = expectedLen - uFRAME_HE_ND_LEN;
                        expectedLen = 0;
                        return resultLen; // 返回完整命令帧长度
                    }
                    else
                    {
                        // 校验失败，丢掉该帧
                        Sr_BufDrop(rngId, expectedLen);
                        state = FB_STATE_FIND_HEAD;
                        available -= expectedLen;
                        expectedLen = 0;
                        break;
                    }
                }
            }
            return 0; // 数据不够，退出函数
        }

        // 如果状态没发生变化（比如还在 FIND_HEAD 但 available 用尽），避免死循环
        if (state == FB_STATE_FIND_HEAD && available < uFRAME_MIN_LEN)
        {
            return 0;
        }
    }
}



/**
 * 应答发送过程
 * ProcTemplateCmd该模板函数可外部文件定义
 * Stm板卡主要为应答
 * 
 */

U16 Cmd2Frm(U8 *pfrm, U8 *pcmd,U16 nByteLen)
{
    U8 nCheckRes = 0;
	U16 i, FrmLen;

	pfrm[0] = FRAME_HEAD;
	pfrm[uFRAME_DEV_IDX] = 0; //设备号，目前请求端为0，响应端为1。
	pfrm[uFRAME_LEN_H_IDX] = (U8)(nByteLen>>8);
	pfrm[uFRAME_LEN_L_IDX] = (U8)(nByteLen&0xFF);

	FrmLen = nByteLen + uFRAME_HEAD_LEN;
	nCheckRes = pfrm[0]^pfrm[1]^pfrm[2]^pfrm[3];
	for(i = uFRAME_HEAD_LEN; i < FrmLen; i++)
	{
		pfrm[i] = pcmd[i-uFRAME_HEAD_LEN];
		nCheckRes ^= pfrm[i];
	}
	pfrm[FrmLen++] = nCheckRes;
	pfrm[FrmLen++] = FRAME_END;
	return FrmLen;
}




//主线程中同步轮询
void ProcPrmFrame(SStaticRngId rngId, U8 asyncMode)
{
	U8 frmType;
	U16 len = 0;

	if((len = FBufferHasExFrame(rngId, m_pReadBuf))> 0)
	{
        if(asyncMode)  //asyncMode=1 入队列，flag=0 直接处理命令
        {
            PushFrameToQueue(m_pReadBuf, len);
        }
        else
        {
            frmType = m_pReadBuf[0];
            switch(frmType)
            {
                case 0x11:	ProcParaCmd(m_pReadBuf, len);	break;	//0x11	参数指令
                case 0x22:	ProcWaveCmd(m_pReadBuf, len);	break;	//0x22	波形指令
                case 0xAA:  ProcTestCmd(m_pReadBuf, len);    break;  //0xAA   测试指令
                //其他非法指令输出应答错误
                default:
                    m_pReadBuf[1] = 0xFF;
                    ProcErrorCmd(m_pReadBuf, len);
                    break;
            }
        }
		
        
	}
}



