#include <stdio.h>
#include "CmdFrm.h"
#include "FrmBuf.h"

//命令接收过程

//接收缓存区考虑使用中断接收
static SStaticRng mRecvRngId;
static U8 m_FrmRcvBuf[MAX_RB_LEN];

static U16 nFrmShortCnt = 0;
static U8 m_pReadBuf[MAX_CMD_LEN];



//全局环形缓存区 mRecvRngId 初始化
void GloabalRingBufInit(void)
{
	Sr_Create(&mRecvRngId, m_FrmRcvBuf, sizeof(m_FrmRcvBuf));
	nFrmShortCnt = 0;
}

U32 PutPrmFrame(const U8 *buf, U32 dataByte)
{
	return Sr_BufPut(&mRecvRngId, buf, dataByte);
}






static U32 FBufferHasExFrame(SStaticRngId rngId, U8 *buffer)
{
	U32 i, len;
	U8 checksum;

	while(Sr_BufGetNoDel(rngId, buffer, uFRAME_MIN_LEN) == uFRAME_MIN_LEN)//有最短帧长可接收
	{
		len = ((U16)buffer[uFRAME_LEN_H_IDX] << 8) | buffer[uFRAME_LEN_L_IDX] + uFRAME_HE_ND_LEN;//完整帧长
		//帧头正确且长度合法
		if((buffer[0] == FRAME_HEAD)
			&& (len >= uFRAME_MIN_LEN) && (len <= uFRAME_MAX_LEN))
		{
			//数据未接收完
			if(Sr_BufGetNoDel(rngId, buffer, len) != len)
			{
				if(++nFrmShortCnt < 250)
				{
					break;
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

/**
 * Todo：
 *  后期可根据命令字直接转换为对应命令结构体，目前先考虑通过buffer直接进行字节流处理。
 *    
 U16 Frm2Cmd(U8 *pData, CmdStruct* cmd, U16 len)
 {
     
 }
 * 
 */



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




U8 ProcTemplateCmd(const U8 *pCmd, U16 len)//pCmd为接收到的命令,len为pCmd的长度，防止越界访问
{
	U16 i,nSendLen, idx = 0;        //从0位置直接开始填入具体数据。
	U8 nDataAns[6] = {0};           //除特殊的命令外，应答基本为6个字节，根据具体反馈命令可修改
	U8 Frm[12] = {0};               //帧头帧尾需要2个字节，设备号1个字节，长度2个字节，校验位1个字节，Len+6；
	nDataAns[idx++] = pCmd[0];      //应答命令字，根据需要接收到的命令进行填写。
    
    /**
     * 根据pCmd中的内容进行填写nDataAns，不同的命令可能需要编写不同的ProcTemplateCmd()函数
	 * nDataAns[idx++] = data1;
	 * nDataAns[idx++] = data2;
     * 内容填写完成后，调用Cmd2Frm()函数转为传输帧
     */

    nSendLen = Cmd2Frm(Frm, nDataAns, idx);
    //写入串口
	//SendCmdAns(Frm, nSendLen);      //写入串口
	return TRUE;
}

U8 ProcParaCmd(const U8 *pCmd, U16 len)
{
    U16 i,nSendLen, idx = 0;        //从0位置直接开始填入具体数据。
    U8 nDataAns[6] = {0};           //除特殊的命令外，应答基本为6个字节，根据具体反馈命令可修改
    U8 Frm[12] = {0};               //帧头帧尾  需要2个字节，设备号1个字节，长度2个字节，校验位1个字节，Len+6；
    nDataAns[idx++] = pCmd[0];      //应答命令字，根据需要接收到的命令进行填写。

    #ifdef DEBUG_PRINT
    printf("ProcParaCmd: ");
    for(i=0;i<len;i++)  
    {
        printf(" %02X",pCmd[i]);
    }
    printf("\n");
    #endif

    nSendLen = Cmd2Frm(Frm, nDataAns, idx);
    //写入串口      
    //SendCmdAns(Frm, nSendLen);
    return TRUE;
}


U8 ProcErrorCmd(const U8 *pCmd, U16 len)
{
    U16 i,nSendLen, idx = 0;        //从0位置直接开始填入具体数据。
    U8 nDataAns[6] = {0};           //除特殊的命令外，应答基本为6个字节，根据具体反馈命令可修改
    U8 Frm[12] = {0};               //帧头帧尾  需要2个字节，设备号1个字节，长度2个字节，校验位1个字节，Len+6；
    nDataAns[idx++] = pCmd[0];      //应答命令字，根据需要接收到的命令进行填写。
    
    #ifdef DEBUG_PRINT
    printf("ProcErrorCmd: ");
    for(i=0;i<len;i++)  
    {
        printf(" %02X",pCmd[i]);
    }
    printf("\n");
    #endif

    nSendLen = Cmd2Frm(Frm, nDataAns, idx);
    //写入串口
    //SendCmdAns(Frm, nSendLen);      //写入串口
    return TRUE;
}

//主线程中轮询
void ProcPrmFrame(void)
{
	U8 frmType;
	U16 len = 0;

	while((len = FBufferHasExFrame(&mRecvRngId, m_pReadBuf))> 0)
	{
		frmType = m_pReadBuf[0];

		switch(frmType)
		{
		    case 0x11:	ProcParaCmd(m_pReadBuf, len);	break;	//0x11	参数指令
		    //case 0x22:	ProcWaveCmd(m_pReadBuf, len);	break;	//0x22	波形指令

		    //其他非法指令输出应答错误
		    default:
			    m_pReadBuf[1] = 0xFF;
			    ProcErrorCmd(m_pReadBuf, len);
			    break;
		}
	}
}