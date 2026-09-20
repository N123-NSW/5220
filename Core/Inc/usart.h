/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
/* USART2 接 485 光编：帧长 5 字节，帧头 0xC0；DMA 普通模式 + 空闲中断（帧间有空闲） */
#define USART2_RX_LEN       32U
#define OPTIC_FRAME_LEN     5U      /* 光编帧长              */
#define OPTIC_FRAME_HEAD    0xC0U   /* 光编帧头              */
#define OPTIC_POS_H_MASK    0x7FU   /* buf[3] 高字节有效位   */

/* USART3：编码器 UART 接口（2.5Mbps，TX DMA 发送 + 空闲中断接收） */
#define USART3_RX_LEN       32U     /* 接收缓冲（最长响应 10 字节，留余量） */
/* USER CODE END Private defines */

void MX_USART2_UART_Init(void);
void MX_USART3_UART_Init(void);

/* USER CODE BEGIN Prototypes */
extern volatile uint8_t  USART2_RxBuf[USART2_RX_LEN];   /* DMA 接收缓冲区 */
extern volatile uint8_t  USART2_RxDone;                 /* 1=已收到完整5字节 */
extern volatile uint16_t USART2_RxLen;                  /* 本帧实际长度 */

/* 485 光编解析结果 */
extern volatile uint32_t g_opticPos;        /* 光编 24bit 位置       */
extern volatile uint8_t  g_opticReady;      /* 1=收到新的光编帧      */
extern volatile uint32_t g_opticFrameCnt;   /* 累计收到的光编帧数    */
extern volatile uint8_t  g_C0flage;         /* 1=收到单字节帧头      */

void USART2_StartRx(void);          /* 启动 RXNE 中断接收 */
void USART2_RxDataCallback(void);   /* RXNE 中断回调，在 USART2_IRQHandler 中调用 */

/* USART3：编码器 UART 接口（2.5Mbps） */
extern volatile uint8_t  USART3_RxBuf[USART3_RX_LEN];  /* 接收缓冲 */
extern volatile uint16_t USART3_RxLen;                 /* 本帧实际长度 */
extern volatile uint8_t  USART3_RxDone;                /* 1=收到一帧响应 */
extern volatile uint8_t  USART3_TxBusy;                /* 1=正在 DMA 发送 */

/* 中转回传：串口3 磁编响应 -> USB 上位机 */
extern volatile uint8_t  g_relayBuf[USART3_RX_LEN];    /* 回传数据缓冲 */
extern volatile uint16_t g_relayLen;                   /* 回传长度 */
extern volatile uint8_t  g_relayFlag;                  /* 1=有待回传的响应 */

/* 光编1字节触发测试：间隔测量 */
extern volatile uint8_t  g_tstTrigEn;                  /* 1=光编1字节立即发CF */
extern volatile uint32_t g_tstTrigCnt;                 /* 触发次数 */
extern volatile uint32_t g_tstOptUs;                   /* 光编字节到达时刻(us) */
extern volatile uint32_t g_tstEncUs;                   /* 磁编响应时刻(us) */
extern volatile uint32_t g_tstIntervalUs;              /* 间隔(us) */
extern volatile uint32_t g_tstEncAngle;                /* 磁编角度(21bit) */

void USART3_StartRx(void);          /* 启动 DMA 接收 + 空闲中断 */
uint8_t USART3_SendData(uint8_t *pData, uint16_t len);  /* 非阻塞 DMA 发送：1=成功 0=忙 */
void USART3_RxIdleCallback(void);   /* 空闲中断回调，在 USART3_IRQHandler 中调用 */
void USART3_TxDoneCallback(void);   /* 发送完成回调，在 DMA1_Channel2_IRQHandler 中调用 */
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

