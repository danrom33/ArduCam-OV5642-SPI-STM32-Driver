#include "main.h"

#include "camera_config.h"

#include <stdio.h>
#include <unistd.h> // Required for _write() syscall function


SPI_HandleTypeDef hspi1;
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef hlpuart1;
DMA_HandleTypeDef hdma_spi1_tx;
DMA_HandleTypeDef hdma_spi1_rx;

HAL_StatusTypeDef SCCB_write_reg(uint16_t reg_addr, uint8_t value);
uint8_t SCCB_read_reg(uint16_t reg_addr);
void ArduChip_write_reg(uint8_t reg_addr, uint8_t val);
uint8_t ArduChip_read_reg(uint8_t reg_addr);
void ArduChip_read_fifo(uint32_t length);


void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_LPUART1_UART_Init(void);

void BspCOM_Init(void);


uint8_t frame_buffer[65535] = {0};
uint32_t jpeg_length = 0;
volatile uint8_t capture_done = 0;

/* Retarget printf/puts to USART2 (USB VCP) */
int _write(int file, char *ptr, int len)
{
  // HAL_UART_Transmit(&hlpuart1, (uint8_t*)ptr, (uint16_t)len, HAL_MAX_DELAY);
    if (file == STDOUT_FILENO || file == STDERR_FILENO) {
        (void)HAL_UART_Transmit(&hlpuart1, (uint8_t*)ptr, (uint16_t)len, HAL_MAX_DELAY);
        return len;
    }
    return -1;
}


int main(void){
    //Reset of all peripherals, initialize the flash interface and Systick
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();
    
    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_I2C1_Init();
    MX_SPI1_Init();
    MX_LPUART1_UART_Init();

    BspCOM_Init();
    // Make printf unbuffered so logs appear immediately
    setvbuf(stdout, NULL, _IONBF, 0);


    HAL_Delay(500);

    //Reset CPLD 
    ArduChip_write_reg(0x07, 0x80); 
    ArduChip_write_reg(0x07, 0x00); 

    uint8_t spi_conn = 0;
    ArduChip_write_reg(0x00, 0x55);

    uint8_t dataRx[2] = {0x3E, 0x3E};
    uint8_t dataTx[2] = {0x00 | 0x80, 0x00};

    while(!spi_conn){
      uint8_t data = ArduChip_read_reg(0x00);
      spi_conn = data == 0x55;
    }

    printf("SPI Connection Tested and Valid\r\n");

    uint8_t id_high = 0;
    uint8_t id_low = 0;
    while((id_high != 0x56) || (id_low != 0x42)){
      id_high = SCCB_read_reg(0x300A);
      id_low = SCCB_read_reg(0x300B);
    }

    printf("I2C Connection Tested and Valid\r\n");

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
    printf("Camera Configured\r\n");

    printf("Transmitting Read Command over SPI...\r\n");

    ArduChip_write_reg(0x03, 0x02); //Set VSync High
    ArduChip_write_reg(0x04, 0x01); //Clear FIFO
    ArduChip_write_reg(0x01, 0x00); //Set to capture 1 frame

    // //Delay for camera configs to sync
    HAL_Delay(500);
    ArduChip_write_reg(0x04, 0x02); //Start capture
    uint8_t done = ArduChip_read_reg(0x41) & 0b00001000; //Is capture done?
    while(!done){
      done = ArduChip_read_reg(0x41) & 0b00001000; //Is capture done?
    }

    // //Get FIFO length
    uint8_t length[3];
    length[2] = ArduChip_read_reg(0x42);
    length[1] = ArduChip_read_reg(0x43);
    length[0] = ArduChip_read_reg(0x44);

    uint32_t fifo_length = (length[2] << 0) + (length[1] << 8) + (length[0] << 16);

    ArduChip_read_fifo(fifo_length);

    //FIFO Uses DMA so asnyc. Can do other stuff here later on...
    while(!capture_done);

    ArduChip_write_reg(0x04, 0x01); //Clear FIFO flag
    
    HAL_Delay(500);
    while(1){
      
    }
}

void ArduChip_read_fifo(uint32_t length)
{
  uint8_t dataTx = 0x3C;

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
  HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(&hspi1, &dataTx, frame_buffer, length);

}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if(hspi == &hspi1)
    {
      
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
      for(int i = 0; i < 65535 - 1; i++)
      {
          if(frame_buffer[i] == 0xFF && frame_buffer[i+1] == 0xD9)
          {
              jpeg_length = i + 2;
              capture_done = 1;
              break;
          }
      }
  }
}


void ArduChip_write_reg(uint8_t reg_addr, uint8_t value){
  uint8_t data[2] = {reg_addr | 0x80, value};
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi1, data, 2, HAL_MAX_DELAY);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
}

uint8_t ArduChip_read_reg(uint8_t reg_addr){
  uint8_t data[2] = {reg_addr, 0x00};
  uint8_t dataRx[2] = {0x33, 0x33};
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
  HAL_SPI_TransmitReceive(&hspi1, data, dataRx, 2, HAL_MAX_DELAY);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
  return dataRx[1];
}

HAL_StatusTypeDef SCCB_write_reg(uint16_t reg_addr, uint8_t value) {

  uint8_t data[3];
  data[0] = (reg_addr>>8) & 0xFF; //high byte of reg addr
  data[1] = reg_addr & 0xFF; //low byte of reg addr
  data[2] = value;

  while(HAL_I2C_IsDeviceReady(&hi2c1, OV5642_WRITE_ADDR, 100, 200) != HAL_OK);
	HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, OV5642_WRITE_ADDR, data, 3, 1000);
  HAL_Delay(10);
  // printf("0x%02x -> Reg 0x%02x\r\n", value, reg_addr);
  return status;
}



uint8_t SCCB_read_reg(uint16_t reg_addr){
  
  uint8_t val;
  uint8_t addr[2];
  addr[0] = (reg_addr >> 8) & 0xFF; //High byte
  addr[1] = (reg_addr & 0xFF); //low byte
  HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, OV5642_WRITE_ADDR, addr, 2, HAL_MAX_DELAY);
  status = HAL_I2C_Master_Receive(&hi2c1, OV5642_READ_ADDR, &val, 1, HAL_MAX_DELAY);
  printf("Received: Reg 0x%02x: 0x%02x\r\n", reg_addr, val);
  return val;
}

/**
  * @brief Retargets the C library printf function to the LPUART1 peripheral (VCP).
  * This is the standard syscall for ARM-GCC.
  * @param file The file descriptor.
  * @param ptr A pointer to the data to send.
  * @param len The number of bytes to send.
  * @return The number of bytes sent, or -1 on error.
  */
/* Bring up the board "COM" (USB VCP) so printf uses the on-board USB port */
void BspCOM_Init(void)
{
    /* Optional: make stdio unbuffered so logs flush immediately */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Voltage scaling */
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
        Error_Handler();
    }

    /* MSI + PLL configuration */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;

    /* MSI = 24 MHz */
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_9;

    /* PLL config */
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;

    /*
       PLL input  = 24 MHz / 3  = 8 MHz
       PLL output = 8 MHz * 20 / 2 = 80 MHz
    */
    RCC_OscInitStruct.PLL.PLLM = 3;
    RCC_OscInitStruct.PLL.PLLN = 20;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;

    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* Clock tree */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_HCLK   |
                                  RCC_CLOCKTYPE_PCLK1  |
                                  RCC_CLOCKTYPE_PCLK2;

    /* SYSCLK = PLL = 80 MHz */
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;

    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }
}


/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00702991;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  hlpuart1.FifoMode = UART_FIFOMODE_DISABLE;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  HAL_PWREx_EnableVddIO2();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);

  /*Configure GPIO pin : PC6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}


void Error_Handler(void)
{
    //Loop forever
    while(1){

    }
}
