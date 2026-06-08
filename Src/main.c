#include "main.h"

#include "camera_config.h"

#include <stdio.h>
#include <unistd.h> // Required for _write() syscall function


I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef hlpuart1;

HAL_StatusTypeDef SCCB_write_reg(uint16_t reg_addr, uint8_t value);
HAL_StatusTypeDef SCCB_read_reg(uint16_t reg_addr);


void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_GPIO_LPUART1_Init(void);
static void MX_LPUART1_UART_Init(void);
void BspCOM_Init(void);
void check_jpeg_length(void);

uint32_t frame_buffer[65535];
uint8_t capture_done = 0;
uint32_t jpeg_length = 0;

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
    BspCOM_Init();
    // Make printf unbuffered so logs appear immediately
    setvbuf(stdout, NULL, _IONBF, 0);

    SCCB_write_reg(0x3008, 0x80); //software reset reg
    for(int i=0; i < sizeof(OV5642_QVGA_Preview)/sizeof(OV5642_QVGA_Preview[0]); i++){
      SCCB_write_reg(OV5642_QVGA_Preview[i][0], OV5642_QVGA_Preview[i][1]);
    }
    for(int i=0; i < sizeof(OV5642_JPEG_Capture_QSXGA)/sizeof(OV5642_JPEG_Capture_QSXGA[0]); i++){
      SCCB_write_reg(OV5642_JPEG_Capture_QSXGA[i][0], OV5642_JPEG_Capture_QSXGA[i][1]);
    }
    for(int i=0; i < sizeof(ov5642_1024x768)/sizeof(ov5642_1024x768[0]); i++){
      SCCB_write_reg(ov5642_1024x768[i][0], ov5642_1024x768[i][1]);
    }
    HAL_Delay(100);
    SCCB_write_reg(0x3818, 0xa8);
    SCCB_write_reg(0x3621, 0x10);
    SCCB_write_reg(0x3801, 0xb0);
    SCCB_write_reg(0x4407, 0x04);
    
    SCCB_read_reg(0x300a);
    SCCB_read_reg(0x300b);
    SCCB_read_reg(0x4300);  // should be 0x30 for JPEG
    SCCB_read_reg(0x4713);  // compression mode, should be 0x03
    SCCB_read_reg(0x3818);  // should be 0xa8 after your write

    printf("Camera Configured\r\nBeginning DCMI Capture\r\n");

    // __HAL_DCMI_DISABLE_IT(&hdcmi, DCMI_IT_OVR);
    HAL_Delay(500);
    while(1){
      
    }
}

void get_jpeg_lenbgth()
{
    // Scan for 0xFF 0xD9 to find true end
    uint8_t *buf = (uint8_t*)frame_buffer;
    for(int i = 0; i < sizeof(frame_buffer) - 1; i++)
    {
        if(buf[i] == 0xFF && buf[i+1] == 0xD9)
        {
            jpeg_length = i + 2;
            break;
        }
    }
    capture_done = 1;
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

    /* MCO = MSI = 24 MHz */
    HAL_RCC_MCOConfig(RCC_MCO1,
                      RCC_MCO1SOURCE_MSI,
                      RCC_MCODIV_1);
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
