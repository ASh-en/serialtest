#pragma once
// #include "typeDefine.h"
// #include "prasingRecvCmd.h"
#ifndef INT16
typedef signed short INT16;
#endif

#ifndef UINT16
typedef unsigned short UINT16;
#endif
#ifndef INT8
typedef signed char INT8;
#endif

#ifndef EOK
#define EOK (0)
#endif // !EOK
#ifndef EMAT_ERROR
#define EMAT_ERROR (-1)
#endif // !EMAT_ERROR

#ifndef SAFE_DELETE
#define SAFE_DELTE(p) do { \
if(NULL != p) { delete p; }\
}while(0)
#endif
#include <vector>
// const int FOREVER = 1;

const int WAVE_POINT_NUM = 1500;
struct PARAM_STRUCT_S
{
	UINT16 index; //参数号
	INT16 value;  //参数值
};

//是否使用曲线平滑 1使用曲线平滑、0不使用
typedef enum
{
	NONE_SMOOTH = 0,
	USING_CURVE_SMOOTH = 1
}IS_CURVE_SMOOTH;
/*回波类型*/
typedef enum
{
	ENVELOPE_WAVE = 0, //包络波
	ORIGIN_WAVE = 1    //原始波
}WAVE_TYPE_E;
/*频率选择*/
typedef enum
{
	LOW_FREQUENCY = 0,    //低频
	HIGH_FREQUENCY = 1    //高频
}FREQUENCY_SELECT_E;

/*测量模式*/
typedef enum
{
	MEASURE_AUTO = 0,        //自动
	MEASURE_HALF_AUTO,		//半自动
	MEASURE_MANUAL,     	//手动
	
	MEASURE_THIN_THIKNESS = 4,   //测薄板模式
}MEASURE_MODE_E;

/*测量模式控制*/
typedef enum
{
	PP_CONTROL_MODE = 0,
	AUTO_CONTROL_MODE =1,
}MEASURE_MODE_CONTROL_E;


/*手动测量模式控制*/
typedef enum
{
	MANUAL_PP_CONTROL_MODE = 0,
	MANUAL_ZP_CONTROL_MODE = 1,
}MANUAL_MEASURE_MODE;

typedef enum
{
	DISPLAY_THICKNESS = 0,
	DISPLAY_WAVE_TYPE = 1,
}LCD_DISPLAY_SWITCH;

typedef enum
{
	SELF_MULTIPLYING = 0,
	ORIGINAL_SIGNAL = 1,
}SELF_MULTIPLY_ALGORITHM;

typedef enum
{
	ALUMINIUM_ALLOY = 0,
	TITANIUM_ALLOY = 1,
}MESURE_MATERIAL_E;

typedef enum
{
	PROBE_WITH_DIRECTION = 0,
	PROBE_WITHOUT_DIRECTION = 1,
}Probe_Direction_Type;

typedef struct 
{
	PARAM_STRUCT_S aluminiumAlloyGainLimit; //0.铝合金版本增益调节上限（200-500，100代表10.0dB）
	PARAM_STRUCT_S zeroPointEnvelope; 		//1.探头零点（包络波） 0-6200，100代表1us
	PARAM_STRUCT_S ultraSpeed;        		//2.超声声速  
	PARAM_STRUCT_S zeroPointOrigin;	  		//3.探头零点（原始波）0-6200，100代表1us
	PARAM_STRUCT_S titaniumAlloyGainLimit; 	//4.钛合金版本增益调节上限 （200-500，100代表10.0dB）
	PARAM_STRUCT_S gate1Start;        		//5.闸门1开始
	PARAM_STRUCT_S gate1End;          		//6.闸门1结束
	PARAM_STRUCT_S calibrationThick;  		//7.标块厚度
	PARAM_STRUCT_S temperature;       		//8.环境温度
	PARAM_STRUCT_S smoothTime;        		//9.平滑次数
	PARAM_STRUCT_S Gain1;        			//10.增益1
	PARAM_STRUCT_S isDirectional;        	//11.探头是否带方向性
	PARAM_STRUCT_S waveType;          		//12.回波类型  WAVE_TYPE_E 0:原始波 1:包络波 2-10 预留
	PARAM_STRUCT_S presetThickness;        	//13.预设厚度（0-10000，1200代表12mm）不带方向探头不适应
	PARAM_STRUCT_S compensationEnable;      //14.补偿使能 0 不补偿 1 按照厚度补偿
	PARAM_STRUCT_S pulseNumber;       		//15.脉冲个数
	PARAM_STRUCT_S excitationFrequency;  //16.激励频率
	PARAM_STRUCT_S measureControlMode;    //17.测量控制模式 0:固定pp模式 其他:自动切换模式，都使用0
	PARAM_STRUCT_S manualMeasureMode;     //18.手动测量控制模式 0:PP模式 1:ZP模式
	PARAM_STRUCT_S selfMultiplying;        //19.自乘算法 0:自乘算法 1:原始信号
	PARAM_STRUCT_S thicknessSmooth;        //20.厚度平滑 5-150
	PARAM_STRUCT_S IsOnMeasurePos;        //21.是否在测量位置 0: 否 ; 1: 是
	PARAM_STRUCT_S measureMode;       	//22.测量模式 0:自动 1:半自动 2:手动 4:测薄板模式	
	PARAM_STRUCT_S isWirelessDownload; 	//23.无线下载是否打开 0: 关闭 1: 打开
	PARAM_STRUCT_S wirelessBaudRate; 	//24.无线模块波特率 24:2400,48:4800,96:9600,1440:14400,1920:19200,3840:38400,5760:57600,1152:115200
	PARAM_STRUCT_S wirelessMode; 		//25.无线模块工作模式 0:点对点A 1:点对点B 2:广播模式
	PARAM_STRUCT_S setWirelessModule; 	//26.设置无线模块 0: 不设置 1: 设置
	PARAM_STRUCT_S wirelessChannel; 	//27.无线模块信道 0-15
	PARAM_STRUCT_S startDisplay;         //28.显示开始
	PARAM_STRUCT_S stopDisplay;          //29.显示结束
	PARAM_STRUCT_S lcdDisplayTxt;        //30.LCD屏显示内容切换 0.显示厚度 1.显示波形
	PARAM_STRUCT_S  measureMaterial;        //31.测厚材料 0.铝 1.钛合金
	PARAM_STRUCT_S  sendWaveSegment;        //32.发送的波形段 0: 5-20us ; 1: 14-29us
	PARAM_STRUCT_S communicateMode;      //33. 通讯模式
	
	
	// PARAM_STRUCT_S curveSmooth;       //9.是否使用曲线平滑 IS_CURVE_SMOOTH
	// PARAM_STRUCT_S ferqSelect;        //16.频率选择 FREQUENCY_SELECT_E
	// PARAM_STRUCT_S SNR;               //15.信噪比
	// PARAM_STRUCT_S dampingRadio;      //16.信号衰减比值
	// PARAM_STRUCT_S envelopeWidth;     //17.波宽
	// PARAM_STRUCT_S widthHeightRadio;  //18.包络波宽高比
	// PARAM_STRUCT_S thresholdValue;    //19.阈值
	// PARAM_STRUCT_S emitInterval;      //20.发射间隔
	// PARAM_STRUCT_S pulseNumber;       //21.脉冲数量
} DEVICE_PARAM_S;

const int PARAM_SIZE = sizeof(DEVICE_PARAM_S) / sizeof(PARAM_STRUCT_S);
//设备仪表参数
union DEVICE_ULTRA_PARAM_U
{
	DEVICE_PARAM_S stParam;
	PARAM_STRUCT_S arrParam[PARAM_SIZE];
};


typedef struct
{
	double time;
	double amp;
}WAVE_POINT_S;

typedef struct
{
	float thick;
	float wave_pos_first;
	float wave_pos_second;
	float curGain;
	float excitation_freq;
	INT16 measureControlMode;
	std::vector<WAVE_POINT_S> pt_vec;
}DISPLAY_WAVE_DATA;

// typedef struct
// {
// 	DISPLAY_WAVE_DATA waveData;
// 	DEVICE_ULTRA_PARAM_U mDeviceParam;
// 	float Thickness;
// 	RECV_COMMAND_NUMBER_E cmdType;
// } Received_data;

