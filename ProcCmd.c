#include "ProcCmd.h"
#include <stdlib.h>
#include "CmdFrm.h"


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
	SendCmdAns(Frm, nSendLen);      //写入串口
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
    SendCmdAns(Frm, nSendLen);
    return TRUE;
}


U8 ProcWaveCmd(const U8 *pCmd, U16 len)//0x22	波形指令
{
    U16 i,nSendLen, idx = 0;        //从0位置直接开始填入具体数据。
    U8 nDataAns[6] = {0};           //除特殊的命令外，应答基本为6个字节，根据具体反馈命令可修改
    U8 Frm[12] = {0};               //帧头帧尾  需要2个字节，设备号1个字节，长度2个字节，校验位1个字节，Len+6；
    nDataAns[idx++] = pCmd[0];      //应答命令字，根据需要接收到的命令进行填写。

    #ifdef DEBUG_PRINT
    printf("ProcWaveCmd: ");
    for(i=0;i<len;i++)  
    {
        printf(" %02X",pCmd[i]);
    }
    printf("\n");
    #endif

    nSendLen = Cmd2Frm(Frm, nDataAns, idx);
    //写入串口
    SendCmdAns(Frm, nSendLen);      //写入串口
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
    SendCmdAns(Frm, nSendLen);      //写入串口
    return TRUE;
}


