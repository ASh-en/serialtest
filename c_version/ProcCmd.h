#ifndef __PROC_CMD_H__
#define __PROC_CMD_H__

#include "types.h"
#include "SerialPort.h"

// 大端 <-> 小端 转换宏
#define SWAP16(x)   ( ((x) >> 8) | ((x) << 8) )

#define SWAP32(x)   ( (((x) >> 24) & 0x000000FF) | \
                      (((x) >>  8) & 0x0000FF00) | \
                      (((x) <<  8) & 0x00FF0000) | \
                      (((x) << 24) & 0xFF000000) )


typedef struct __attribute__((packed))
{
    U8  cmd;        // 命令码：0x11
    U8  rwFlag;     // 读写标志：0x55：读，0xAA：写, 0x5A：重置
    U8  paraId;
    U16 paraVal[];  // 柔性数组，可变长度
    // padding 固定最后一个字节
} sParaCmd;

typedef struct __attribute__((packed))
{
    U8 cmd;       ///< 0x11
    U8 rwFlag;    ///< 0x55
    U8 paraId;    ///< 参数编号
    U8 filler;    ///< 0xA5
} sPara4BCmd;

typedef struct __attribute__((packed))
{
    U8 cmd;        ///< 0x11
    U8 rwFlag;     ///< 0xAA
    U8 paraId;     ///< 参数编号
    U16 paraVal;   ///< 待写入值，大端
    U8 filler;     ///< 0xA5
} sParaWrite6BCmd;

typedef struct __attribute__((packed))
{
    U8  cmd;         ///< 0x11
    U8  rwFlag;      ///< 0xAA
    U8  paraId;      ///< 0xFF表示全部参数
    U16 paraVal[34]; ///< 34个参数，大端存储
    U8  filler;      ///< 0xA5
} sParaWriteAllCmd;


typedef struct __attribute__((packed))
{
    U8 cmd;       ///< 命令字 0x11
    U8 rwFlag;    ///< 0x55=读取 / 0xAA=写入 / 0x5A=重置
    U8 paraId;    ///< 参数编号 0x00~0x21 或 0xFF=全部参数
    union
    {
        U16 val;        ///< 读取单个参数值
        struct
        {
            U8 result;    ///< 写入/重置结果 0x33/0xCC
            U8 filler;    ///< 写入/重置填充 0xA5
        } res;
    } param;             ///< 参数值或操作结果
    U8 filler;          ///< 填充 0xA5
} sPara6BResp;

typedef struct __attribute__((packed))
{
    U8  cmd;           ///< 0x11
    U8  rwFlag;        ///< 0x55
    U8  paraId;        ///< 0xFF表示全部参数
    U16 paraVal[34];   ///< 34个参数值，高位在前
    U8  filler;        ///< 0xA5
} sParaReadAllResp;



typedef struct __attribute__((packed))
{
    U8  cmd;          ///< 命令字 0x22
    U8  frameNo;      ///< 帧序号
    U8  totalFrames;  ///< 总帧数
    U16 totalLen;     ///< 总长度，大端
    U8  chk;          ///< 校验码
    S16 desc[6];      ///< 波形整体描述数据，高位在前
} sWaveInfoResp;

typedef struct __attribute__((packed))
{
    U8  frameNo;      ///< 帧序号
    U16 dataOffset;   ///< 数据偏移，大端
    S16 data[48];     ///< 波形数据，N最大为96字节 = 48个S16
} sWaveDataResp;

typedef struct __attribute__((packed))
{
    U8  frameNo;      ///< 0xFF
    U8  sentFrames;   ///< 已发帧数
    U16 sentLen;      ///< 已发长度，大端
    U8  chk;          ///< 校验码
} sWaveAckResp;



typedef struct __attribute__((packed))
{
    U8  cmd;        // 命令码：0x22
    U8  subCmd;     // 子命令码： 0x55：请求波形， 0xAA：确认反馈
    U32 ask;        // 请求波形时采用0xA5A5A5A5A5A5填充，  确认反馈时：根据实际接收帧进行填充
} sWaveCmd;

typedef struct __attribute__((packed))
{
    U8  cmd;        // 命令码：0x33
    U8  subCmd;     // 子命令码： 0x11：标定， 0x55：开始请求， 0xAA：停止请求
    U16  data;      // 数据值 (mm*100 / 发送频率 / 0xA5A5)
}sThickCmd;

typedef struct __attribute__((packed))
{
    U8 cmd;        ///< 命令字 0x33
    U8 subCode;    ///< 子功能码 0x11/0x55/0xAA
    U8 result;     ///< 操作结果 0x33=成功, 0xCC=失败
    U16 timestamp; ///< 时间戳，大端模式
    U8 filler;     ///< 填充字 0xA5
} sThickResp;



typedef struct __attribute__((packed))
{
    U8  cmd;        ///< 命令字 (固定 0x35)
    U8  seq;        ///< 包序号 (0x00~0xFF 循环)
    U16 thickness;  ///< 厚度值 (大端存储, 单位: mm*100)
    U16 timestamp;  ///< 时间戳 (大端存储)
} sThickData;


typedef struct __attribute__((packed))
{
    U8  cmd;        ///< 命令字 (0x41)
    U8  dirFlag;    ///< 上下行标志 (请求端 0x55)
    U16 timestamp;  ///< 时间戳 (大端存储)
} sTimeCmd;

typedef struct __attribute__((packed))
{
    U8  cmd;        ///< 命令字 0x41
    U8  dirFlag;    ///< 响应端 0xAA
    U16 timestamp;  ///< 时间戳，大端模式
    U8  result;     ///< 操作结果标志 0x33=成功, 0xCC=失败
    U8  filler;     ///< 填充字 0xA5
} sTimeResp;



typedef struct __attribute__((packed))
{
    U8  cmd;       ///< 命令字 (0x42)
    U8  subCode;   ///< 子功能码 (0x01 表示软件版本查询, 0x02 表示硬件版本查询)
    U16 timestamp; ///< 填充字段 (大端模式，目前无实际作用)
} sVerCmd;

typedef struct __attribute__((packed))
{
    U8 cmd;        ///< 命令字 0x42
    U8 subCode;    ///< 0xFF 硬件编号写入
    U8 version;    ///< M/R
    U8 mainCtrl;   ///< A/B
    U8 commType;   ///< S/W
    U8 burnMode;   ///< 0/1/2
    U8 year;
    U8 month;
    U8 day;
    U8 serialNo;   ///< 流水号
} sHardwareWriteCmd;


typedef struct __attribute__((packed))
{
    U8 cmd;       ///< 命令字 0x42
    U8 subCode;   ///< 子功能码 (0x01/0x02/0xFF)
    
    union
    {
        struct __attribute__((packed))
        {
            U16 versionType; ///< 版本类型 ST/DV/EX
            U8  productLine; ///< L/T
            U8  commType;    ///< S/W
            U8  year;
            U8  month;
            U8  day;
            U8  buildNo;
        } baseInfo;         ///< 子功能码 0x01

        struct __attribute__((packed))
        {
            U8 version;       ///< M/R
            U8 mainCtrl;      ///< A/B
            U8 commModule;    ///< S/W
            U8 burnMode;      ///< 烧录方式 0/1/2
            U8 year;
            U8 month;
            U8 day;
            U8 serialNo;      ///< 流水号
        } moduleInfo;       ///< 子功能码 0x02 / 0xFF
    };
} sVerResp;


typedef struct __attribute__((packed))
{
    U8  cmd;        ///< 命令字 0x44
    U16 timestamp;  ///< 时间戳，大端
    U8  filler;     ///< 填充字 0xA5
} sBatCmd;

typedef struct __attribute__((packed))
{
    U8  cmd;        ///< 命令字 0x44
    U8  battery;    ///< 电量 0~100
    U16 timestamp;  ///< 大端时间戳
    U16 filler;     ///< 填充字 0xA5A5
} sBatResp;





S32 ProcTemplateCmd(const U8 *pCmd, U32 len);        //模板命令处理函数
S32 ProcParaCmd(const U8 *pCmd, U32 len);            //参数命令
S32 ProcWaveCmd(const U8 *pCmd, U32 len);            //波形命令
S32 ProcErrorCmd(const U8 *pCmd, U32 len);           //错误命令
S32 ProcTestCmd(const U8 *pCmd, U32 len);            //测试命令  
void SendCmdAns(SerialPort* sp, const U8 *pBuf, S32 byteLen);



#endif