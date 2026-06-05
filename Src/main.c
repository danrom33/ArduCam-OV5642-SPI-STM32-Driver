#include "main.h"

#include "camera_config.h"

#include <stdio.h>
#include <unistd.h> // Required for _write() syscall function

DMA_HandleTypeDef hdma_spi1_tx;
DMA_HandleTypeDef hdma_spi1_rx;
I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi2;
UART_HandleTypeDef huart2;

HAL_StatusTypeDef SCCB_write_reg(uint16_t reg_addr, uint8_t value);
HAL_StatusTypeDef SCCB_read_reg(uint16_t reg_addr);
HAL_StatusTypeDef ArduChip_write_reg(uint8_t addr, uint8_t value);
uint8_t ArduChip_read_reg(uint8_t addr);
HAL_StatusTypeDef ArduChip_read_fifo(uint8_t length[]);


void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART2_UART_Init(void);
void BspCOM_Init(void);

uint8_t dataRx[65535];
uint8_t SPIBusy = 0;
int capture_done = 0;
uint32_t jpeg_length = 0;

/* Retarget printf/puts to USART2 (USB VCP) */
int _write(int file, char *ptr, int len)
{
  // HAL_UART_Transmit(&hlpuart1, (uint8_t*)ptr, (uint16_t)len, HAL_MAX_DELAY);
    if (file == STDOUT_FILENO || file == STDERR_FILENO) {
        (void)HAL_UART_Transmit(&huart2, (uint8_t*)ptr, (uint16_t)len, HAL_MAX_DELAY);
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
    MX_SPI2_Init();
    BspCOM_Init();
    // Make printf unbuffered so logs appear immediately
    setvbuf(stdout, NULL, _IONBF, 0);

    char msg[] = "Testing testing";
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, HAL_MAX_DELAY);

    uint8_t dataTx[2];
    dataTx[0] = 0x00;
    dataTx[1] = 0x00;
    uint8_t dataReceive[2] = {0xff};
    printf("Beginning SPI transmission");
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi2, dataTx, dataReceive, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);

   

    while(1){
    }
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef * hspi)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
    // ArduChip_write_reg(0x04, 0x01);
    SPIBusy = 0;
    // Scan for 0xFF 0xD9 to find true end
    for(int i = 0; i < sizeof(dataRx) - 1; i++)
    {
        if(dataRx[i] == 0xFF && dataRx[i+1] == 0xD9)
        {
            jpeg_length = i + 2;
            break;
        }
    }
    capture_done = 1;
}

HAL_StatusTypeDef ArduChip_read_fifo(uint8_t length[]){
  uint32_t fifo_length = (length[0] << 16) + (length[1] << 8) + length[2] + 1; //Add 1 for dummy byte
  uint16_t dma_tx_length = (fifo_length < 65535) ? fifo_length : 65535;
  static uint8_t tx = 0x3C;
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
  SPIBusy = 1;
  HAL_SPI_TransmitReceive_DMA(&hspi2, &tx, dataRx, dma_tx_length);

}

HAL_StatusTypeDef ArduChip_write_reg(uint8_t addr, uint8_t value) {
  uint8_t dataTx[2];
  dataTx[0] = addr | 0x80;
  dataTx[1] = value;
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
  HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi2, dataTx, 2, HAL_MAX_DELAY);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
  return status;
}

uint8_t ArduChip_read_reg(uint8_t addr) {
  uint8_t dataTx[2];
  dataTx[0] = addr & ~0x80;
  dataTx[1] = 0x00;
  uint8_t dataReceive[2] = {0xff};
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
  HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi2, dataTx, dataReceive, 2, HAL_MAX_DELAY);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
  return dataReceive[1];
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
    MX_USART2_UART_Init();

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
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 3;
  RCC_OscInitStruct.PLL.PLLN = 20;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

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
