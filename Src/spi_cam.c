#include "spi_cam.h"
#include "camera_config.h"
#include "sccb.h"
#include "arduchip.h"

#include <stdio.h>

OV5642_HandleTypeDef hov5642;

uint8_t *buffer;
uint32_t jpeg_length = 0;
volatile uint8_t fifo_read_complete = 0;

OV5642_StatusTypeDef OV5642_TakePicture(uint8_t frame_buffer[], uint32_t *image_length){
    buffer = frame_buffer;
    //Reset read_complete flag
    fifo_read_complete = 0;
    ArduChip_write_reg(0x04, 0x01); //Clear FIFO flag
    ArduChip_write_reg(0x01, 0x00); //Set to capture 1 frame

    uint8_t reg_val;

    ArduChip_write_reg(0x04, 0x02); //Start capture
    ArduChip_read_reg(0x41, &reg_val); //Is capture done?
    while(!(reg_val & 0b00001000)){
      ArduChip_read_reg(0x41, &reg_val); //Is capture done?
    }

    // //Get FIFO length
    uint8_t length[3];
    ArduChip_read_reg(0x42, &reg_val);
    length[2] = reg_val;
    ArduChip_read_reg(0x43, &reg_val);
    length[1] = reg_val;
    ArduChip_read_reg(0x44, &reg_val);
    length[0] = reg_val;

    uint32_t fifo_length = (length[2] << 0) + (length[1] << 8) + (length[0] << 16);

    ArduChip_read_fifo(fifo_length, frame_buffer);

    //FIFO Uses DMA so asnyc. Can do other stuff here later on...
    while(!fifo_read_complete);
    //Reset read_complete flag
    fifo_read_complete = 0;
    ArduChip_write_reg(0x04, 0x01); //Clear FIFO flag
    *image_length = jpeg_length;
    return OV5642_OK;
}

OV5642_StatusTypeDef OV5642_Init(I2C_HandleTypeDef *hi2c, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_gpio_port, uint16_t cs_gpio_pin){
  hov5642.hi2c = hi2c;
  hov5642.hspi = hspi;
  hov5642.cs_gpio_port = cs_gpio_port;
  hov5642.cs_gpio_pin = cs_gpio_pin;
  return OV5642_OK;
}

OV5642_StatusTypeDef OV5642_SetJpeg(){
  SCCB_write_reg(0x3008, 0x80); //ov5642 reset reg 
  HAL_Delay(100);
  
  Write_SensorConfigs(OV5642_QVGA_Preview);
  Write_SensorConfigs(OV5642_JPEG_Capture_QSXGA);
 
  SCCB_write_reg(0x3818, 0xa8);
  SCCB_write_reg(0x3621, 0x10);
  SCCB_write_reg(0x3801, 0xb0);
  SCCB_write_reg(0x4407, 0x08);
  SCCB_write_reg(0x5888, 0x00);
  SCCB_write_reg(0x5000, 0xFF);

  return OV5642_OK;
}

OV5642_StatusTypeDef OV5642_SetResolution(OV5642_Resolution res){
  switch(res){
    case OV5642_320x240:
      Write_SensorConfigs(ov5642_320x240);
      return OV5642_OK;
    case OV5642_640x480:
      Write_SensorConfigs(ov5642_640x480);
      return OV5642_OK;
    case OV5642_1024x768:
      Write_SensorConfigs(ov5642_1024x768);
      return OV5642_OK;
    case OV5642_1280x960:
      Write_SensorConfigs(ov5642_1280x960);
      return OV5642_OK;
    case OV5642_1600x1200:
      Write_SensorConfigs(ov5642_1600x1200);
      return OV5642_OK;
    case OV5642_2048x1536:
      Write_SensorConfigs(ov5642_2048x1536);
      return OV5642_OK;
    case OV5642_2592x1944:
      Write_SensorConfigs(ov5642_2592x1944);
      return OV5642_OK;
    default:
      return OV5642_ERROR;
  }
}

OV5642_StatusTypeDef OV5642_TestSerialConnection(){

    uint8_t reg_val;
   //Reset CPLD 
    ArduChip_write_reg(0x07, 0x80); 
    ArduChip_write_reg(0x07, 0x00); 

    uint8_t spi_conn = 0;
    uint8_t attempts = 0;
    ArduChip_write_reg(0x00, 0x55);

    while(attempts < 10 && !spi_conn){
      ArduChip_read_reg(0x00, &reg_val);
      spi_conn = reg_val == 0x55;
      attempts++;
      HAL_Delay(100);
    }
    if(attempts == 10)
      return OV5642_SPI_TIMEOUT;
    attempts = 0;

    // printf("SPI Connection Tested and Valid\r\n");

    uint8_t id_high = 0;
    uint8_t id_low = 0;
    while(attempts < 10){
      SCCB_read_reg(0x300A, &reg_val);
      id_high = reg_val;
      SCCB_read_reg(0x300B, &reg_val);
      id_low = reg_val;
      if((id_high == 0x56) && (id_low == 0x42))
        break;
      attempts++;
      HAL_Delay(100);
    }
    if(attempts == 10)
      return OV5642_I2C_TIMEOUT;
    return OV5642_OK;
}



void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if(hspi == hov5642.hspi)
    {
      
      HAL_GPIO_WritePin(hov5642.cs_gpio_port, hov5642.cs_gpio_pin, GPIO_PIN_SET);
      for(int i = 0; i < 65535 - 1; i++)
      {
          if(buffer[i] == 0xFF && buffer[i+1] == 0xD9)
          {
              jpeg_length = i + 2;
              fifo_read_complete = 1;
              break;
          }
      }
  }
}

static void Write_SensorConfigs(const uint16_t config[][2]){
  uint16_t i = 0;

  while(!(config[i][0] == 0xffff && config[i][1] == 0xff)) {
    SCCB_write_reg(config[i][0], config[i][1]);
    i++;
  }
}