#include "sccb.h"
#include "camera_config.h"

extern OV5642_HandleTypeDef hov5642;

OV5642_StatusTypeDef SCCB_write_reg(uint16_t reg_addr, uint8_t value) {

  uint8_t data[3];
  data[0] = (reg_addr>>8) & 0xFF; //high byte of reg addr
  data[1] = reg_addr & 0xFF; //low byte of reg addr
  data[2] = value;

  if(HAL_I2C_IsDeviceReady(hov5642.hi2c, OV5642_WRITE_ADDR, 100, 200) != HAL_OK){
    return OV5642_I2C_TIMEOUT;
  }
  HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hov5642.hi2c, OV5642_WRITE_ADDR, data, 3, 1000);
  if(status != HAL_OK){
    if(status == HAL_TIMEOUT)
        return OV5642_I2C_TIMEOUT;
    return OV5642_ERROR;
  }
  HAL_Delay(10);
  return OV5642_OK;
}



OV5642_StatusTypeDef SCCB_read_reg(uint16_t reg_addr, uint8_t *val){
  uint8_t addr[2];
  addr[0] = (reg_addr >> 8) & 0xFF; //High byte
  addr[1] = (reg_addr & 0xFF); //low byte
  if(HAL_I2C_IsDeviceReady(hov5642.hi2c, OV5642_WRITE_ADDR, 100, 200) != HAL_OK){
    return OV5642_I2CTIMEOUT;
  }
  HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hov5642.hi2c, OV5642_WRITE_ADDR, addr, 2, HAL_MAX_DELAY);
   if(status != HAL_OK){
    if(status == HAL_TIMEOUT)
        return OV5642_I2CTIMEOUT;
    return OV5642_ERROR;
  }
  status = HAL_I2C_Master_Receive(hov5642.hi2c, OV5642_READ_ADDR, val, 1, HAL_MAX_DELAY);
   if(status != HAL_OK){
    if(status == HAL_TIMEOUT)
        return OV5642_I2CTIMEOUT;
    return OV5642_ERROR;
  }
//   printf("Received: Reg 0x%02x: 0x%02x\r\n", reg_addr, val);
  return OV5642_OK;
}