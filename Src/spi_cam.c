#include "spi_cam.h"
#include "camera_config.h"
#include "sccb.h"
#include "arduchip.h"


static OV5642_HandleTypeDef hov5642;

uint8_t frame_buffer[65535] = {0};
uint32_t jpeg_length = 0;
volatile uint8_t fifo_read_complete = 0;

int take_picture(void){
    
    HAL_Delay(500);

    uint8_t reg_val;

    //Reset CPLD 
    ArduChip_write_reg(0x07, 0x80); 
    ArduChip_write_reg(0x07, 0x00); 

    uint8_t spi_conn = 0;
    ArduChip_write_reg(0x00, 0x55);

    uint8_t dataRx[2] = {0x3E, 0x3E};
    uint8_t dataTx[2] = {0x00 | 0x80, 0x00};

    while(!spi_conn){
      ArduChip_read_reg(0x00, reg_val);
      spi_conn = reg_val == 0x55;
    }

    // printf("SPI Connection Tested and Valid\r\n");

    uint8_t id_high = 0;
    uint8_t id_low = 0;
    while((id_high != 0x56) || (id_low != 0x42)){
      SCCB_read_reg(0x300A, reg_val);
      id_high = reg_val;
      SCCB_read_reg(0x300B, reg_val);
      id_low = reg_val;
    }

    // printf("I2C Connection Tested and Valid\r\n");

    SCCB_write_reg(0x3008, 0x80); //ov5642 reset reg 

    for(int i=0; i < sizeof(OV5642_QVGA_Preview)/sizeof(OV5642_QVGA_Preview[0]); i++){
      SCCB_write_reg(OV5642_QVGA_Preview[i][0], OV5642_QVGA_Preview[i][1]);
    }
    HAL_Delay(100);
    HAL_Delay(100);
    for(int i=0; i < sizeof(OV5642_JPEG_Capture_QSXGA)/sizeof(OV5642_JPEG_Capture_QSXGA[0]); i++){
      SCCB_write_reg(OV5642_JPEG_Capture_QSXGA[i][0], OV5642_JPEG_Capture_QSXGA[i][1]);
    }
    for(int i=0; i < sizeof(ov5642_640x480)/sizeof(ov5642_640x480[0]); i++){
      SCCB_write_reg(ov5642_640x480[i][0], ov5642_640x480[i][1]);
    }
    HAL_Delay(100);
    SCCB_write_reg(0x3818, 0xa8);
    SCCB_write_reg(0x3621, 0x10);
    SCCB_write_reg(0x3801, 0xb0);
    SCCB_write_reg(0x4407, 0x08);
    SCCB_write_reg(0x5888, 0x00);
    SCCB_write_reg(0x5000, 0xFF);

    HAL_Delay(100);
    // printf("Camera Configured\r\n");

    // printf("Transmitting Read Command over SPI...\r\n");

    ArduChip_write_reg(0x03, 0x02); //Set VSync High
    ArduChip_write_reg(0x04, 0x01); //Clear FIFO
    ArduChip_write_reg(0x01, 0x00); //Set to capture 1 frame

    // //Delay for camera configs to sync
    HAL_Delay(500);
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

    ArduChip_write_reg(0x04, 0x01); //Clear FIFO flag
    
    HAL_Delay(500);
    while(1){
      
    }
}

HAL_StatusTypeDef OV5642_Init(I2C_HandleTypeDef *hi2c, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_gpio_port, uint16_t *cs_gpio_pin){
  hov5642.hi2c = hi2c;
  hov5642.hspi = hspi;
  hov5642.cs_gpio_port = cs_gpio_port;
  hov5642.cs_gpio_pin = cs_gpio_pin;
  return HAL_OK;
}



void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if(hspi == hov5642.hspi)
    {
      
      HAL_GPIO_WritePin(hov5642.cs_gpio_port, hov5642.cs_gpio_pin, GPIO_PIN_SET);
      for(int i = 0; i < 65535 - 1; i++)
      {
          if(frame_buffer[i] == 0xFF && frame_buffer[i+1] == 0xD9)
          {
              jpeg_length = i + 2;
              fifo_read_complete = 1;
              break;
          }
      }
  }
}

void Error_Handler(void)
{
    //Loop forever
    while(1){

    }
}
