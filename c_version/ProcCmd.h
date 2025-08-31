#ifndef __PROC_CMD_H__
#define __PROC_CMD_H__

#include "types.h"
#include "SerialPort.h"




S32 ProcTemplateCmd(const U8 *pCmd, U32 len);        //模板命令处理函数
S32 ProcParaCmd(const U8 *pCmd, U32 len);            //参数命令
S32 ProcWaveCmd(const U8 *pCmd, U32 len);            //波形命令
S32 ProcErrorCmd(const U8 *pCmd, U32 len);           //错误命令
S32 ProcTestCmd(const U8 *pCmd, U32 len);            //测试命令  
void SendCmdAns(SerialPort* sp, const U8 *pBuf, S32 byteLen);



#endif