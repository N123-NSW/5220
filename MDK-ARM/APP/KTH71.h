#ifndef __KTH71_H__
#define __KTH71_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "main.h"
#include "APP.h"

#define KTH71_READ_ANGLE                        0x00
#define KTH71_READ_REG                          0x11
#define KTH71_WRITE_REG                         0x33

uint8_t KTH71_ReadReg(uint8_t addr, uint8_t *pRegValue);
uint8_t KTH71_WriteReg(uint8_t addr, uint8_t data);
void KTH71_UnlockReg(void);
void KTH71_LockReg(void);
void KTH71_WriteRegToMTP(void);
uint8_t KTH71_ReadAngle(uint16_t *pAngle);


#ifdef __cplusplus
}
#endif
#endif /*__KTH71_H__ */
