#include "arduchip.h"

extern OV5642_HandleTypeDef hov5642;

OV5642_StatusTypeDef ArduChip_write_reg(uint8_t reg_addr, uint8_t value){
  uint8_t data[2] = {reg_addr | 0x80, value};
  CS_Select();
  HAL_StatusTypeDef status = HAL_SPI_Transmit(hov5642.hspi, data, 2, HAL_MAX_DELAY);
  CS_Deselect();
  if(status != HAL_OK){
    if(status == HAL_TIMEOUT)
        return OV5642_SPI_TIMEOUT;
    return OV5642_ERROR;
  }
  return OV5642_OK;
}

OV5642_StatusTypeDef ArduChip_read_reg(uint8_t reg_addr, uint8_t *val){
  uint8_t data[2] = {reg_addr, 0x00};
  uint8_t dataRx[2] = {0x33, 0x33};
  CS_Select();
  HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(hov5642.hspi, data, dataRx, 2, HAL_MAX_DELAY);
  CS_Deselect();
  if(status != HAL_OK){
    if(status == HAL_TIMEOUT)
        return OV5642_SPI_TIMEOUT;
    return OV5642_ERROR;
  }
  *val = dataRx[1];
  return OV5642_OK;
}


OV5642_StatusTypeDef ArduChip_read_fifo(uint32_t length)
{
  CS_Select();
  HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(hov5642.hspi, &dataTx, frame_buffer, length);
  return OV5642_OK;
}

static void CS_Select(){
    HAL_GPIO_WritePin(hov5642.cs_gpio_port, hov5642.cs_gpio_pin, GPIO_PIN_RESET);
}

static void CS_Select(){
    HAL_GPIO_WritePin(hov5642.cs_gpio_port, hov5642.cs_gpio_pin, GPIO_PIN_SET);
}