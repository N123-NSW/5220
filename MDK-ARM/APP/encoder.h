#ifndef __encoder_H__
#define __encoder_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "main.h"
#include "APP.h"
typedef struct
{
    uint32_t angle;       /* 16bit 单圈绝对位置  */
    uint8_t  angle_crc;    /* encoderCRC 校验结果 */

    uint8_t  EncLevel_flag;  /* 编码器电平转换标志 */
    uint8_t  almc;          /* 故障字节 */

    uint8_t  angle_ready;   /* 是否有新角度可读 */
} Encoder_t;

#ifdef __cplusplus
}
#endif
#endif /*__encoder_H__ */

