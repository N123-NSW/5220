/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "usart.h"
#include "host_proto.h"

/* USER CODE BEGIN 0 */
/* =====================================================================
 *  USART2：485 光编接收（4Mbps，帧头 0xC0，DMA 普通模式 + 空闲中断）
 * ===================================================================== */
volatile uint8_t  USART2_RxBuf[USART2_RX_LEN];   /* DMA 接收缓冲 */
volatile uint8_t  USART2_RxDone = 0;             /* 1=收到完整 5 字节帧 */
volatile uint16_t USART2_RxLen  = 0;             /* 本帧长度（1 或 5） */
volatile uint32_t g_opticPos = 0;                /* 光编 24bit 位置 */
volatile uint8_t  g_opticReady = 0;              /* 1=收到新位置帧 */
volatile uint8_t  g_C0flage = 0;                 /* 1=收到单字节帧头 */

/* 光编 RXNE 状态机（简单方案：收到0xC0单字节立即发CF，收5字节角度帧）
 * 状态：0=等单字节(发CF), 1=收角度帧(5字节) */
static volatile uint8_t s_optState = 0;
static volatile uint8_t s_optIdx = 0;
static volatile uint8_t s_optBuf[OPTIC_FRAME_LEN];

/* =====================================================================
 *  USART3：编码器 UART 接口（2.5Mbps，TX DMA 发送 + 空闲中断接收）
 * ===================================================================== */
volatile uint8_t  USART3_RxBuf[USART3_RX_LEN];   /* DMA 接收缓冲 */
volatile uint16_t USART3_RxLen  = 0;             /* 本帧长度 */
volatile uint8_t  USART3_RxDone = 0;             /* 1=收到一帧响应 */
volatile uint8_t  USART3_TxBusy = 0;             /* 1=正在 DMA 发送 */

/* 中转回传缓冲：串口3 磁编响应 -> USB 上位机（中断写入，主循环读取后 USB 发送） */
volatile uint8_t  g_relayBuf[USART3_RX_LEN];     /* 回传数据缓冲 */
volatile uint16_t g_relayLen  = 0;               /* 回传长度 */
volatile uint8_t  g_relayFlag = 0;               /* 1=有待回传的响应 */

/* =====================================================================
 *  采集误差（上位机 0xCC 命令）：2400 组 [光编角度 + 磁编角度]
 *  同步策略：串口2 收到 1 字节同步信号 -> 触发串口3 发 0x02 读角度
 * ===================================================================== */
uint32_t          g_optAngle[ERR_SAMPLE_NUM];    /* 光编角度(串口2) */
uint32_t          g_encAngle[ERR_SAMPLE_NUM];    /* 磁编角度(串口3) */
volatile uint8_t  g_errState = ERR_STATE_IDLE;   /* 采集状态 */
volatile uint16_t g_errCount = 0;                /* 已采集组数 */

/* 开始采集：复位计数，进入采集中状态，光编状态机收到单字节即发CF */
void ErrSample_Start(void)
{
    g_errCount = 0;
    g_errState = ERR_STATE_SAMPLING;
    s_optState = 0;   /* 采集触发：收到单字节(0xC0)就发CF */
    s_optIdx = 0;
}

/* ---------- 内部辅助：清 USART 错误/空闲标志（防 ORE 锁死） ---------- */
static void USART_ClearError(USART_TypeDef *USARTx)
{
    if(LL_USART_IsActiveFlag_ORE(USARTx))
    {
        (void)LL_USART_ReceiveData8(USARTx);   /* 先读 RDR 再清 ORE */
    }
    USARTx->ICR = USART_ICR_ORECF | USART_ICR_NCF | USART_ICR_FECF | USART_ICR_PECF | USART_ICR_IDLECF;
}

/* ---------- 内部辅助：停 DMA 通道并等 EN 清零 ---------- */
static void DMA_Stop(uint32_t Channel)
{
    LL_DMA_DisableChannel(DMA1, Channel);
    while(LL_DMA_IsEnabledChannel(DMA1, Channel));
}

/* ---------- 直接写 TDR 发送1字节（最快路径，省 DMA 配置） ---------- */
static inline void USART3_SendByte(uint8_t byte)
{
    while(!LL_USART_IsActiveFlag_TXE(USART3));  /* 等 TDR 空（1字节@2.5M=4us） */
    LL_USART_TransmitData8(USART3, byte);       /* 写 TDR，立即开始发送 */
}

/* =====================================================================
 *  USART2 接收（RXNE 每字节中断，最快响应；不用 DMA + IDLE）
 * ===================================================================== */
void USART2_StartRx(void)
{
    LL_USART_SetTransferDirection(USART2, LL_USART_DIRECTION_TX_RX); /* 开 RE */

    USART_ClearError(USART2);
    LL_USART_EnableIT_RXNE(USART2);   /* 使能 RXNE 中断：每收到1字节立即触发 */

    /* 重置光编状态机 */
    s_optState = 0;
    s_optIdx = 0;
}

/* =====================================================================
 *  USART3 接收
 * ===================================================================== */
void USART3_StartRx(void)
{
    DMA_Stop(LL_DMA_CHANNEL_3);
    LL_DMA_ClearFlag_TC3(DMA1);
    LL_DMA_ClearFlag_TE3(DMA1);

    USART_ClearError(USART3);
    LL_USART_EnableIT_IDLE(USART3);

    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_3, USART3_RX_LEN);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_3);
}

/* =====================================================================
 *  USART3 发送（非阻塞，TC 中断置完成标志；外设地址在 Init 配置）
 * ===================================================================== */
uint8_t USART3_SendData(uint8_t *pData, uint16_t len)
{
    if(USART3_TxBusy) return 0;

    DMA_Stop(LL_DMA_CHANNEL_2);
    LL_DMA_ClearFlag_TC2(DMA1);
    LL_DMA_ClearFlag_TE2(DMA1);

    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_2);

    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_2, (uint32_t)pData);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, len);

    USART3_TxBusy = 1;
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);
    return 1;
}

/* DMA1_Channel2 发送完成中断回调 */
void USART3_TxDoneCallback(void)
{
    LL_DMA_ClearFlag_TC2(DMA1);
    USART3_TxBusy = 0;
}

/* ===== 光编1字节触发测试：DWT 微秒计时 + 间隔测量 ===== */
/* DWT 硬件由 main.c 的 DWT_Init 初始化，这里只读 cycle counter */
static inline uint32_t DWT_GetUs(void)
{
    return DWT->CYCCNT / (SystemCoreClock / 1000000UL);
}

volatile uint8_t  g_tstTrigEn     = 0;   /* 0=关闭自动发CF；1=光编收到1字节立即发CF(测试) */
volatile uint32_t g_tstTrigCnt    = 0;   /* 触发次数（光编1字节） */
volatile uint32_t g_tstOptUs      = 0;   /* 光编1字节到达时刻(us) */
volatile uint32_t g_tstEncUs      = 0;   /* 磁编响应到达时刻(us) */
volatile uint32_t g_tstIntervalUs = 0;   /* 间隔 = 磁编-光编(us) */
volatile uint32_t g_tstEncAngle   = 0;   /* 磁编角度(21bit) */
/* USER CODE END 0 */

/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */
  /* USER CODE END USART2_Init 0 */

  LL_USART_InitTypeDef USART_InitStruct = {0};
 
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOD);
  /**USART2 GPIO Configuration
  PD5   ------> USART2_TX
  PD6   ------> USART2_RX
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_5|LL_GPIO_PIN_6;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USART2 DMA Init */

  /* USART2_RX Init */
  LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_6, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

  LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PRIORITY_HIGH);

  LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MODE_NORMAL);

  LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PDATAALIGN_BYTE);

  LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MDATAALIGN_BYTE);

  LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_6, (uint32_t)&USART2->RDR);

  LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_6, (uint32_t)USART2_RxBuf);

  /* USART2 interrupt Init */
  NVIC_SetPriority(USART2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),1, 0));
  NVIC_EnableIRQ(USART2_IRQn);

  /* USER CODE BEGIN USART2_Init 1 */
  /* USER CODE END USART2_Init 1 */
  USART_InitStruct.BaudRate = 4000000;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART2, &USART_InitStruct);
  LL_USART_DisableIT_CTS(USART2);
  LL_USART_ConfigAsyncMode(USART2);
  LL_USART_Enable(USART2);
  /* USER CODE BEGIN USART2_Init 2 */
  /* RXNE 中断方案：不使用 DMA 接收，改为每字节 RXNE 中断（最快响应）。
   * 注意：不在此处启动接收。Init 阶段只发不收(RE=0)，避免光编上电前
   * 485 总线噪声。待 main.c 里光编电源使能后再调用 USART2_StartRx()。 */
  /* USER CODE END USART2_Init 2 */

}
/* USART3 init function */

void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */
  /* USER CODE END USART3_Init 0 */

  LL_USART_InitTypeDef USART_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOE);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
  /**USART3 GPIO Configuration
  PE15   ------> USART3_RX
  PB10   ------> USART3_TX
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_15;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USART3 DMA Init */

  /* USART3_TX Init */
  LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_2, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

  LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PRIORITY_LOW);

  LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MODE_NORMAL);

  LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PDATAALIGN_BYTE);

  LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MDATAALIGN_BYTE);

  LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_2, (uint32_t)&USART3->TDR);

  /* USART3_RX Init (DMA1_Channel3：USART3_RX，外设→内存) */
  LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_3, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

  LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_3, LL_DMA_PRIORITY_HIGH);

  LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_3, LL_DMA_MODE_NORMAL);

  LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_3, LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_3, LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_3, LL_DMA_PDATAALIGN_BYTE);

  LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_3, LL_DMA_MDATAALIGN_BYTE);

  LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_3, (uint32_t)&USART3->RDR);

  LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_3, (uint32_t)USART3_RxBuf);

  /* USART3 interrupt Init */
  NVIC_SetPriority(USART3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(USART3_IRQn);

  /* USER CODE BEGIN USART3_Init 1 */
  /* USER CODE END USART3_Init 1 */
  USART_InitStruct.BaudRate = 2500000;       /* 编码器 UART：2.5Mbps（72MHz/28.8，误差约0.04%） */
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART3, &USART_InitStruct);
  LL_USART_DisableIT_CTS(USART3);
  LL_USART_ConfigAsyncMode(USART3);
  LL_USART_Enable(USART3);
  /* USER CODE BEGIN USART3_Init 2 */
  /* 使能 TX/RX DMA 请求 */
  LL_USART_EnableDMAReq_TX(USART3);
  LL_USART_EnableDMAReq_RX(USART3);

  /* 启动串口3 DMA 接收（设置数据长度 + 使能通道 + 使能空闲中断）。
   * 缺少这一步磁编响应不会被 DMA 搬走，上位机就收不到回传数据。 */
  USART3_StartRx();
  /* USER CODE END USART3_Init 2 */

}

/* USER CODE BEGIN 1 */
/* USART2 收到1字节(RXNE)中断处理：简单方案，收到单字节0xC0立即发CF */
void USART2_RxDataCallback(void)
{
    uint8_t ch;

    /* 溢出/错误：清错误 + 重置状态机，防止接收锁死 */
    if(LL_USART_IsActiveFlag_ORE(USART2) || LL_USART_IsActiveFlag_NE(USART2) || LL_USART_IsActiveFlag_FE(USART2))
    {
        USART_ClearError(USART2);
        s_optState = 0;
        s_optIdx = 0;
        return;
    }

    ch = LL_USART_ReceiveData8(USART2);   /* 读 RDR，清 RXNE */

    if(s_optState == 0)
    {
        /* 收到字节立即发 CF，不判断字节值（单字节值不固定，判断会漏发/延迟） */
        g_C0flage = 1;

        if(g_errState == ERR_STATE_SAMPLING)
        {
            USART3_SendByte(HOST_CMD_READ_ANGLE);
        }
        else if(g_tstTrigEn)
        {
            g_tstOptUs = DWT_GetUs();
            g_tstTrigCnt++;
            USART3_SendByte(HOST_CMD_READ_ANGLE);
        }

        s_optState = 1;   /* 转去收角度帧 */
        s_optIdx = 0;
    }
    else
    {
        /* 收角度帧：5字节（帧头0xC0 + 4字节位置） */
        s_optBuf[s_optIdx++] = ch;
        if(s_optIdx >= OPTIC_FRAME_LEN)
        {
            if(s_optBuf[0] == OPTIC_FRAME_HEAD)
            {
                g_opticPos = ((uint32_t)(s_optBuf[3] & OPTIC_POS_H_MASK) << 16)
                           | ((uint32_t)s_optBuf[2] << 8)
                           |  (uint32_t)s_optBuf[1];
                g_opticReady = 1;
                USART2_RxDone = 1;
                USART2_RxLen  = OPTIC_FRAME_LEN;
                /* 采集模式：存光编位置 */
                if(g_errState == ERR_STATE_SAMPLING)
                {
                    g_optAngle[g_errCount] = g_opticPos;
                }
            }
            s_optState = 0;   /* 回等单字节 */
        }
    }
}

void USART3_RxIdleCallback(void)
{
    uint16_t rxLen;

    if(LL_USART_IsActiveFlag_ORE(USART3) || LL_USART_IsActiveFlag_NE(USART3) || LL_USART_IsActiveFlag_FE(USART3))
    {
        USART_ClearError(USART3);
        USART3_RxLen  = 0;
        USART3_RxDone = 0;
        USART3_StartRx();
        return;
    }

    if(!LL_USART_IsActiveFlag_IDLE(USART3)) return;

    rxLen = (uint16_t)(USART3_RX_LEN - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_3));
		if(rxLen == 0)
    {
        USART3_StartRx();
        return;
    }
    LL_USART_ClearFlag_IDLE(USART3);

    DMA_Stop(LL_DMA_CHANNEL_3);

    USART3_RxLen  = rxLen;
    USART3_RxDone = 1;

    /* 测试模式：记录磁编响应到达时刻，算间隔，解析角度 */
    if(g_tstTrigEn)
    {
        g_tstEncUs = DWT_GetUs();
        g_tstIntervalUs = g_tstEncUs - g_tstOptUs;   /* 光编1字节 -> 磁编响应 总间隔(us) */
        if((rxLen >= 6) && (USART3_RxBuf[0] == HOST_CMD_READ_ANGLE))
        {
            g_tstEncAngle = (uint32_t)USART3_RxBuf[2]
                          | ((uint32_t)USART3_RxBuf[3] << 8)
                          | (((uint32_t)USART3_RxBuf[4] & 0x1FU) << 16);
        }
    }

    /* 采集模式：解析磁编角度（21位，LSB先出：DF0|DF1<<8|DF2<<16），配对计数 */
    if(g_errState == ERR_STATE_SAMPLING)
    {
        if((rxLen >= 6) && (USART3_RxBuf[0] == HOST_CMD_READ_ANGLE))
        {
            g_encAngle[g_errCount] = (uint32_t)USART3_RxBuf[2]
                                   | ((uint32_t)USART3_RxBuf[3] << 8)
                                   | (((uint32_t)USART3_RxBuf[4] & 0x1FU) << 16);
            g_errCount++;
            if(g_errCount >= ERR_SAMPLE_NUM)
            {
                g_errState = ERR_STATE_DONE;   /* 采集完成 */
            }
        }
    }
    else
    {
        /* 普通中转模式：把磁编响应原样拷贝到回传缓冲，由主循环通过 USB 回传给上位机 */
        uint16_t i;
        uint16_t n = (rxLen < USART3_RX_LEN) ? rxLen : USART3_RX_LEN;
        for(i = 0; i < n; i++)
        {
            g_relayBuf[i] = USART3_RxBuf[i];
        }
        g_relayLen  = n;
        g_relayFlag = 1;
    }

    USART3_StartRx();
}
/* USER CODE END 1 */

