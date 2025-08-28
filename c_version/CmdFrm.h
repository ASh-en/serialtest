#ifndef __CMDFRM_H__
#define __CMDFRM_H__
#include "types.h"

#define FRAME_HEAD  (0x66)
#define FRAME_END (0x99)

#define MAX_RB_LEN              (0x0400)				//环形缓存区长度
#define MAX_CMD_LEN         	(0x65)				    //最大业务命令长度
#define MIN_CMD_LEN         	(0x04)				    //最小业务命令长度
#define uFRAME_HEAD_LEN			(4)					    //传输帧头长度（1字节帧头+1字节设备号+2字节长度）
#define uFRAME_END_LEN			(2)					    //传输帧尾长度（1字节校验码+1字节帧尾）

#define uFRAME_DEV_IDX          (1)                     //设备号序号
#define uFRAME_LEN_H_IDX        (2)                     //长度高位序号
#define uFRAME_LEN_L_IDX        (3)                     //长度低位序号

#define uFRAME_HE_ND_LEN		(uFRAME_HEAD_LEN + uFRAME_END_LEN)				//传输帧结构长度
#define uFRAME_MIN_LEN 			(uFRAME_HEAD_LEN+MIN_CMD_LEN+uFRAME_END_LEN)	//传输帧最小长度
#define uFRAME_MAX_LEN			(uFRAME_HEAD_LEN+MAX_CMD_LEN+uFRAME_END_LEN)	//传输帧最大长度

void GloabalRingBufInit(void);
S32 PutPrmFrame(const U8 *buf, S32 dataByte);
void ProcPrmFrame(void);
void SendCmdAns(const U8 *pBuf, S32 byteLen);
U16 Cmd2Frm(U8 *pfrm, U8 *pcmd,U16 nByteLen);

#endif/*__CMDFRM_H__*/