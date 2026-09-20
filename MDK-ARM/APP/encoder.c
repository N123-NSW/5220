#include "encoder.h"
Encoder_t encoder;
void Encoder_Power(void)
{
	if(encoder.EncLevel_flag == 0)
	{
		LL_GPIO_SetOutputPin(GPIOE,LL_GPIO_PIN_10);
		LL_GPIO_ResetOutputPin(GPIOB,LL_GPIO_PIN_2);
		LL_GPIO_SetOutputPin(GPIOF,LL_GPIO_PIN_4);
	}
	else if (encoder.EncLevel_flag  == 1)
	{
		LL_GPIO_ResetOutputPin(GPIOE,LL_GPIO_PIN_10);
		LL_GPIO_SetOutputPin(GPIOB,LL_GPIO_PIN_2);
		LL_GPIO_SetOutputPin(GPIOF,LL_GPIO_PIN_4);
	}
	//3V3D-3V3S
		LL_GPIO_SetOutputPin(GPIOE,LL_GPIO_PIN_2);
		LL_GPIO_SetOutputPin(GPIOE,LL_GPIO_PIN_9);
	//MCU_OE:SPIOE
		LL_GPIO_SetOutputPin(GPIOC,LL_GPIO_PIN_4);
}
