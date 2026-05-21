#include "main.h"

#include "camera_config.h"

#include <stdio.h>
#include <unistd.h> // Required for _write() syscall function

DCMI_HandleTypeDef hdcmi;
DMA_HandleTypeDef hdma_dcmi;
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef hlpuart1;

HAL_StatusTypeDef SCCB_write_reg(uint8_t reg_addr, uint8_t value);
HAL_StatusTypeDef SCCB_read_reg(uint8_t reg_addr);


void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_DCMI_Init(void);
static void MX_I2C1_Init(void);
static void MX_GPIO_LPUART1_Init(void);
static void MX_LPUART1_UART_Init(void);
void BspCOM_Init(void);

uint32_t frame_buffer[QCIF_ROWS*QCIF_COLUMNS/2];

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
    MX_DCMI_Init();
    MX_I2C1_Init();
    BspCOM_Init();
    // Make printf unbuffered so logs appear immediately
    setvbuf(stdout, NULL, _IONBF, 0);

    SCCB_write_reg(0x12, 0x80); 
    HAL_Delay(1000);
    for(int i=0; i < sizeof(OV7670_QCIF_UYVY)/sizeof(OV7670_QCIF_UYVY[0]); i++){
      HAL_Delay(20);
      SCCB_write_reg(OV7670_QCIF_UYVY[i][0], OV7670_QCIF_UYVY[i][1]);
    }
    // for(int i=0; i < sizeof(OV7670_TestPattern)/sizeof(OV7670_TestPattern[0]); i++){
    //   HAL_Delay(20);
    //   SCCB_write_reg(OV7670_TestPattern[i][0], OV7670_TestPattern[i][1]);
    // }
    // for(int i=0; i < sizeof(OV7670_QCIF_UYVY)/sizeof(OV7670_QCIF_UYVY[0]); i++){
    //   SCCB_read_reg(OV7670_QCIF_UYVY[i][0]);
    // }

    HAL_Delay(20);
    printf("Camera Configured\r\nBeginning DCMI Capture\r\n");

    __HAL_DCMI_DISABLE_IT(&hdcmi, DCMI_IT_OVR);

    HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)frame_buffer, QCIF_ROWS*QCIF_COLUMNS/2);

    while(1){
      
    }
}

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi){
  printf("Frame Captured\r\n");
  printf("Words Remaining: %d/12672", hdma_dcmi.Instance->CNDTR);
}

void HAL_DCMI_ErrorCallback(DCMI_HandleTypeDef *hdcmi){
  printf("Error");
}

HAL_StatusTypeDef SCCB_write_reg(uint8_t reg_addr, uint8_t value) {

  uint8_t data[2];
  data[0] = reg_addr;
  data[1] = value;

  while(HAL_I2C_IsDeviceReady(&hi2c1, OV7670_WRITE_ADDR, 100, 200) != HAL_OK);
	HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, OV7670_WRITE_ADDR, data, 2, 1000);
  // printf("0x%02x -> Reg 0x%02x\r\n", value, reg_addr);
  HAL_Delay(100);
  return status;
}

HAL_StatusTypeDef SCCB_read_reg(uint8_t reg_addr){
  
  uint8_t val;
  HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, OV7670_WRITE_ADDR, &reg_addr, 1, HAL_MAX_DELAY);
  status = HAL_I2C_Master_Receive(&hi2c1, OV7670_READ_ADDR, &val, 1, HAL_MAX_DELAY);
  printf("Received: Reg 0x%02x: 0x%02x\r\n", reg_addr, val);
  return status;
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
    MX_GPIO_LPUART1_Init();
    MX_LPUART1_UART_Init();

    /* Optional: make stdio unbuffered so logs flush immediately */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_9;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_SYSCLK, RCC_MCODIV_2);
}

/**
  * @brief DCMI Initialization Function
  * @param None
  * @retval None
  */
static void MX_DCMI_Init(void)
{

  /* USER CODE BEGIN DCMI_Init 0 */

  /* USER CODE END DCMI_Init 0 */

  /* USER CODE BEGIN DCMI_Init 1 */

  /* USER CODE END DCMI_Init 1 */
  hdcmi.Instance = DCMI;
  hdcmi.Init.SynchroMode = DCMI_SYNCHRO_HARDWARE;
  hdcmi.Init.PCKPolarity = DCMI_PCKPOLARITY_RISING;
  hdcmi.Init.VSPolarity = DCMI_VSPOLARITY_HIGH;
  hdcmi.Init.HSPolarity = DCMI_HSPOLARITY_LOW;
  hdcmi.Init.CaptureRate = DCMI_CR_ALL_FRAME;
  hdcmi.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
  hdcmi.Init.JPEGMode = DCMI_JPEG_DISABLE;
  hdcmi.Init.ByteSelectMode = DCMI_BSM_ALL;
  hdcmi.Init.ByteSelectStart = DCMI_OEBS_ODD;
  hdcmi.Init.LineSelectMode = DCMI_LSM_ALL;
  hdcmi.Init.LineSelectStart = DCMI_OELS_ODD;
  if (HAL_DCMI_Init(&hdcmi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DCMI_Init 2 */

  /* USER CODE END DCMI_Init 2 */

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
  hi2c1.Init.Timing = 0x10805D88;
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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  HAL_PWREx_EnableVddIO2();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}


/* LPUART on PG7 (TX) / PG8 (RX) for ST-LINK VCP (same USB used to flash) */
static void MX_GPIO_LPUART1_Init(void)
{
    HAL_PWREx_EnableVddIO2();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin       = GPIO_PIN_7 | GPIO_PIN_8; /* PG7 TX, PG8 RX */
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_LPUART1;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
}

static void MX_LPUART1_UART_Init(void)
{
  __HAL_RCC_LPUART1_CLK_ENABLE();
  
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
}

void Error_Handler(void)
{
    //Loop forever
    while(1){

    }
}
