/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_custom_hid_if.c
  * @version        : v2.0_Cube
  * @brief          : USB Device Custom HID interface file.
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

/* Includes ------------------------------------------------------------------*/
#include "usbd_custom_hid_if.h"
#include "host_proto.h"
#include "usart.h"

/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device.
  * @{
  */

/** @addtogroup USBD_CUSTOM_HID
  * @{
  */

/** @defgroup USBD_CUSTOM_HID_Private_TypesDefinitions USBD_CUSTOM_HID_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Defines USBD_CUSTOM_HID_Private_Defines
  * @brief Private defines.
  * @{
  */

/* USER CODE BEGIN PRIVATE_DEFINES */

/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Macros USBD_CUSTOM_HID_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Variables USBD_CUSTOM_HID_Private_Variables
  * @brief Private variables.
  * @{
  */

/** Usb HID report descriptor. */
__ALIGN_BEGIN static uint8_t CUSTOM_HID_ReportDesc_FS[USBD_CUSTOM_HID_REPORT_DESC_SIZE] __ALIGN_END =
{
  /* USER CODE BEGIN 0 */
	0x05,0x01,        // 0x05(Usage-Page)  0x01 �� ͨ�������豸ҳ(Generic?Desktop�������Զ����豸�������ҳ)
	0x09,0x00,        // 0x09(Usage) 0x00 �� �豸��;�������ⶨ�壬�Զ���HID�豸
	0xA1,0x01,        // 0xA1(Collection) 0x01 �� ������Ӧ�ü��ϡ�������HID���Ķ�������ڴ˼�����

	// IN �˵� �豸 �� �����������ϱ�64�ֽ����ݰ�
	0x05,0x01,        // Usage-Page������ѡ��ͨ������ҳ��
	0x09,0x30,        // Usage X-Axis X�ᣬ����IN�ϱ�ͨ����ʶ
	0x15,0x00,        // �߼���Сֵ����ֵ���� 0
	0x25,0xFF,        // �߼����ֵ����ֵ���� 255(���ֽ����ֵ)
	0x75,0x08,        // Report-Size �����С��ÿ�����ݵ�Ԫռ8bit = 1�ֽ�
	0x95,0x40,        // Report-Count �������������ݵ�Ԫ������0x40=64��һ���ϱ�64���ֽ�
	0x81,0x02,        // Input �������־λ=0x02
				   // Data(����)��Variable(ÿ���ֽڶ�������)��Absolute(������ֵ)

	// OUT �˵� �������� �� ��λ���豸���·�64�ֽ����ݰ�
	0x05,0x01,        // Usage-Page ͨ������ҳ
	0x09,0x31,        // Usage Y-Axis Y�ᣬ����OUT�·�ͨ����ʶ
	0x15,0x00,        // �·��ֽ���Сֵ 0
	0x25,0xFF,        // �·��ֽ����ֵ 255
	0x75,0x08,        // ÿ����Ԫ 8λ 1�ֽ�
	0x95,0x40,        // һ��64����Ԫ�����ν���64�ֽ�
	0x91,0x02,        // Output ����Data��Variable��Absolute
				   // �������豸�������ݰ�����
  /* USER CODE END 0 */
  0xC0    /*     END_COLLECTION	             */
};

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Exported_Variables USBD_CUSTOM_HID_Exported_Variables
  * @brief Public variables.
  * @{
  */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */
/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_FunctionPrototypes USBD_CUSTOM_HID_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t CUSTOM_HID_Init_FS(void);
static int8_t CUSTOM_HID_DeInit_FS(void);
static int8_t CUSTOM_HID_OutEvent_FS(uint8_t event_idx, uint8_t state);

/**
  * @}
  */

USBD_CUSTOM_HID_ItfTypeDef USBD_CustomHID_fops_FS =
{
  CUSTOM_HID_ReportDesc_FS,
  CUSTOM_HID_Init_FS,
  CUSTOM_HID_DeInit_FS,
  CUSTOM_HID_OutEvent_FS
};

/** @defgroup USBD_CUSTOM_HID_Private_Functions USBD_CUSTOM_HID_Private_Functions
  * @brief Private functions.
  * @{
  */

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes the CUSTOM HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_Init_FS(void)
{
  /* USER CODE BEGIN 4 */
  return (USBD_OK);
  /* USER CODE END 4 */
}

/**
  * @brief  DeInitializes the CUSTOM HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_DeInit_FS(void)
{
  /* USER CODE BEGIN 5 */
  return (USBD_OK);
  /* USER CODE END 5 */
}

/**
  * @brief  Manage the CUSTOM HID class events
  * @param  event_idx: Event index
  * @param  state: Event state
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_OutEvent_FS(uint8_t event_idx, uint8_t state)
{
  /* USER CODE BEGIN 6 */
    HostCmd_t *pCmd = (HostCmd_t *)hUsbDeviceFS.pClassData;

    /* 校验帧头 0xAA + 异或 CRC */
    if(pCmd->head != HOST_FRAME_HEAD) return USBD_OK;
    if(pCmd->crc != (uint8_t)(pCmd->head ^ pCmd->cmd ^ pCmd->param)) return USBD_OK;

    /* 采集中：忽略新命令，保证采集不被打扰 */
    if(g_errState == ERR_STATE_SAMPLING) return USBD_OK;

    switch(pCmd->cmd)
    {
        case HOST_CMD_ERR_SAMPLE:               /* 采集误差：开始采集 */
            ErrSample_Start();
            break;

        /* 普通命令：串口3 发送对应 CF */
        case HOST_CMD_READ_ANGLE:
        case HOST_CMD_READ_ID:
        case HOST_CMD_READ_ALL:
        case HOST_CMD_FAST_CAL:
        case HOST_CMD_SLOW_CAL:
        case HOST_CMD_FAULT_RST:
        {
            static uint8_t cmd;                 /* static 保证 DMA 发送期间内存有效 */
            cmd = pCmd->cmd;
            USART3_SendData(&cmd, 1);
            break;
        }

        default:
            break;
    }

    return (USBD_OK);
  /* USER CODE END 6 */
}

/* USER CODE BEGIN 7 */
/* ---------- 采集数据上传（每包 64 字节 = 16 个 uint32，小端） ---------- */
static uint32_t s_uploadIdx   = 0;   /* 当前包起始索引 */
static uint8_t  s_uploadPhase = 0;   /* 0=光编, 1=磁编, 2=完成 */

/**
  * @brief  上传一包采集数据，返回 1=还有数据，0=全部上传完成
  */
uint8_t HID_UploadNextPacket(void)
{
    uint8_t  report[64];
    uint32_t *src;
    uint16_t n, i;

    if(s_uploadPhase >= 2) return 0;

    src = (s_uploadPhase == 0) ? g_optAngle : g_encAngle;
    n   = ERR_SAMPLE_NUM - s_uploadIdx;
    if(n > 16) n = 16;

    /* uint32 -> 小端字节流，补齐剩余 */
    for(i = 0; i < 16; i++)
    {
        uint32_t v = (i < n) ? src[s_uploadIdx + i] : 0UL;
        report[i*4 + 0] = (uint8_t)(v);
        report[i*4 + 1] = (uint8_t)(v >> 8);
        report[i*4 + 2] = (uint8_t)(v >> 16);
        report[i*4 + 3] = (uint8_t)(v >> 24);
    }

    /* USB 忙则本次不发，返回"还有数据"让主循环下一轮重试（不递增索引，避免丢包） */
    if(USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, report, 64) != USBD_OK)
    {
        return 1U;
    }

    s_uploadIdx += n;
    if(s_uploadIdx >= ERR_SAMPLE_NUM)
    {
        s_uploadIdx = 0;
        s_uploadPhase++;
    }

    return (s_uploadPhase < 2) ? 1U : 0U;
}

/* 上传完成复位（采集状态回空闲） */
void HID_UploadReset(void)
{
    s_uploadIdx   = 0;
    s_uploadPhase = 0;
}

/* ---------- 中转回传：串口3 磁编响应 -> USB 上位机 ---------- */
/**
  * @brief  把磁编(串口3)的响应原样回传给 USB 上位机
  * @note   HID 报告固定 64 字节，响应不足部分填 0；上位机按响应首字节(CF)判断有效长度
  * @retval 1=发送成功, 0=USB 忙(未发送，需重试)
  */
uint8_t HID_RelayResponse(uint8_t *pData, uint16_t len)
{
    uint8_t  report[64];
    uint16_t i;

    if(len > 64) len = 64;

    for(i = 0; i < 64; i++)
    {
        report[i] = (i < len) ? pData[i] : 0x00U;
    }

    return (USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, report, 64) == USBD_OK) ? 1U : 0U;
}
/* USER CODE END 7 */

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

