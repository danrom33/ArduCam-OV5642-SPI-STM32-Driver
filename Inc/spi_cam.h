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
    OV5642_320x240 = 0,
    OV5642_640x480,
    OV5642_1024x768,
    OV5642_1280x960,
    OV5642_1600x1200,
    OV5642_2048x1536,
    OV5642_2592x1944
} OV5642_Resolution;

typedef enum {
    OV5642_OK = 0,
    OV5642_ERROR,
    OV5642_SPI_TIMEOUT,
    OV5642_I2C_TIMEOUT,
} OV5642_StatusTypeDef;

OV5642_StatusTypeDef OV5642_Init(I2C_HandleTypeDef *hi2c, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_gpio_port, uint16_t cs_gpio_pin);
OV5642_StatusTypeDef OV5642_TakePicture(uint8_t frame_buffer[], uint32_t *image_length);
OV5642_StatusTypeDef OV5642_SetJpeg();
OV5642_StatusTypeDef OV5642_SetResolution(OV5642_Resolution res);
OV5642_StatusTypeDef OV5642_TestSerialConnection();


void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __SPI_CAM_H */
