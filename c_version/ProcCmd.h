#ifndef __PROC_CMD_H__
#define __PROC_CMD_H__

#include "types.h"
#include "SerialPort.h"




U8 ProcTemplateCmd(const U8 *pCmd, U16 len);        //模板命令处理函数
U8 ProcParaCmd(const U8 *pCmd, U16 len);            //参数命令
U8 ProcWaveCmd(const U8 *pCmd, U16 len);            //波形命令
U8 ProcErrorCmd(const U8 *pCmd, U16 len);           //错误命令
U8 ProcTestCmd(const U8 *pCmd, U16 len);            //测试命令  
void SendCmdAns(SerialPort* sp, const U8 *pBuf, S32 byteLen);



#endif