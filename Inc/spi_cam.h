#ifndef __SPI_CAM_H
#define __SPI_CAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l4xx_hal.h"

typedef struct {
    I2C_HandleTypeDef *hi2c;
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_gpio_port;
    uint16_t cs_gpio_pin;
} OV5642_HandleTypeDef;

typedef enum {
    OV5642_OK = 0,
    OV5642_ERROR,
    OV5642_SPI_TIMEOUT,
    OV5642_I2C_TIMEOUT,
} OV5642_StatusTypeDef;

OV5642_StatusTypeDef OV5642_Init(I2C_HandleTypeDef *hi2c, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_gpio_port, uint16_t cs_gpio_pin);

OV5642_StatusTypeDef OV5642_Take_Picture(uint8_t frame_buffer[], uint32_t *length);


void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __SPI_CAM_H */
