/**
  ******************************************************************************
  * @file    host_proto.h
  * @brief   上位机(USB HID) 命令协议定义 + 采集误差缓冲声明
  ******************************************************************************
  */
#ifndef __HOST_PROTO_H__
#define __HOST_PROTO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ================= 上位机命令帧（USB HID OUT，4 字节） =================
 * 帧格式：0xAA(帧头) + CF(命令码) + 0x00(预留参数) + 异或CRC
 * 异或CRC = 帧头 ^ 命令码 ^ 参数
 */
#define HOST_FRAME_HEAD      0xAAU

/* CF 命令码 */
typedef enum
{
    HOST_CMD_READ_ANGLE = 0x02,   /* 读单圈位置    Req=[CF]             Resp=[CF+SF+DF0+DF1+DF2+CRC] */
    HOST_CMD_READ_ID    = 0x92,   /* 读 ID         Req=[CF]             Resp=[CF+SF+DF0+CRC]         */
    HOST_CMD_READ_ALL   = 0x1A,   /* 读所有信息    Req=[CF]             Resp=[CF+SF+DF0..DF7+CRC]    */
    HOST_CMD_FAST_CAL   = 0x03,   /* 快速自校准    Req=[CF]             Resp=[CF+CorrectionSatus]    */
    HOST_CMD_SLOW_CAL   = 0x04,   /* 慢校准        Req=[CF]             Resp=[CF+CorrectionSatus]    */
    HOST_CMD_WRITE_EEP  = 0x32,   /* 写 E2PROM     Req=[CF+ADF+EDF+CRC] Resp=[CF+ADF+EDF+CRC]        */
    HOST_CMD_READ_EEP   = 0xEA,   /* 读 E2PROM     Req=[CF+ADF+CRC]     Resp=[CF+ADF+EDF+CRC]        */
    HOST_CMD_FAULT_RST  = 0xBA,   /* 故障复位      Req=[CF]             Resp=[CF+SF+DF0+DF1+DF2+CRC] */
    HOST_CMD_ERR_SAMPLE = 0xCC,   /* 采集误差(新增) */
} HostCmd_e;

/* 上位机命令帧 */
typedef struct
{
    uint8_t head;   /* 0xAA 固定帧头 */
    uint8_t cmd;    /* CF 命令码 */
    uint8_t param;  /* 0x00 预留参数 */
    uint8_t crc;    /* 异或校验 = head ^ cmd ^ param */
} HostCmd_t;

/* ================= 采集误差 ================= */
#define ERR_SAMPLE_NUM        2400U     /* 采集组数 */

typedef enum
{
    ERR_STATE_IDLE = 0,                 /* 空闲 */
    ERR_STATE_SAMPLING,                 /* 采集中（期间不可打扰） */
    ERR_STATE_DONE,                     /* 采集完成，待上传 */
} ErrState_e;

/* 采集数据（全局，采集完成由 USB 分批上传） */
extern uint32_t          g_optAngle[ERR_SAMPLE_NUM];  /* 光编角度(串口2) */
extern uint32_t          g_encAngle[ERR_SAMPLE_NUM];  /* 磁编角度(串口3) */
extern volatile uint8_t  g_errState;                  /* 采集状态 ErrState_e */
extern volatile uint16_t g_errCount;                  /* 已采集组数 */

/* 采集控制 */
void ErrSample_Start(void);   /* 开始采集（复位计数，进入采集中） */

#ifdef __cplusplus
}
#endif

#endif /* __HOST_PROTO_H__ */
