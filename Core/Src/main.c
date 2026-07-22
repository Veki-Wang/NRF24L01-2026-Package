/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : NRF24L01 双机通信测试
  ******************************************************************************
  * @attention
  * 两块板子，每块插一个 NRF24L01。
  * 改下面 DEVICE_MODE，分别编译烧录：
  *   MODE_TX → 板A（发送端）
  *   MODE_RX → 板B（接收端）
  * 都上电后，TX端OLED数据递增，RX端OLED显示相同数据 → 通信成功
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "NRF24L01.h"
#include "OLED.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* ★★★ 改这里切换角色 ★★★ */
#define MODE_TX  1
#define MODE_RX  2
#define DEVICE_MODE  MODE_RX   // <-- 先烧TX，再改RX烧另一个板子
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#if DEVICE_MODE == MODE_TX
uint8_t  tx_data[4];        // 要发送的数据
uint8_t  send_flag;         // 1=成功 2=失败 3=异常 4=超时
uint16_t send_ok;           // 发送成功次数
uint16_t send_fail;         // 发送失败次数
uint32_t last_send_ms;      // 上次发送时刻
#endif

#if DEVICE_MODE == MODE_RX
uint8_t  rx_data[4];        // 接收到的数据
uint8_t  recv_flag;         // 1=成功 2=异常 3=掉电
uint16_t recv_ok;           // 接收成功次数
uint16_t recv_fail;         // 接收失败次数
#endif

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_I2C2_Init();

  /* USER CODE BEGIN 2 */

  /* ---- LED闪3次：证明代码在跑 ---- */
  for(int i = 0; i < 3; i++)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_Delay(200);
  }

  /* ---- OLED初始化 + I2C快速自检 ---- */
  OLED_Init();
  OLED_Clear();
  {
    uint8_t test_cmd = 0x00;
    if(HAL_I2C_Mem_Write(&hi2c2, 0x78, 0x00, I2C_MEMADD_SIZE_8BIT,
                          &test_cmd, 1, 100) != HAL_OK)
    {
      /* I2C不通：LED快闪10次后卡住 */
      for(int i = 0; i < 10; i++)
      {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
        HAL_Delay(50);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_Delay(50);
      }
      while(1);
    }
  }

  /* ---- NRF24L01初始化 + 自检 ---- */
  NRF24L01_Init();
  {
    uint8_t cv = NRF24L01_ReadReg(NRF24L01_CONFIG);
    if(cv == 0xFF || cv == 0x00)
    {
      OLED_ShowString(0, 0, "NRF ERR!", OLED_8X16);
      OLED_Update();
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); // LED常亮
      while(1);
    }
  }

  /* ---- OLED显示界面 ---- */
  OLED_Clear();
#if DEVICE_MODE == MODE_TX
  OLED_ShowString(0, 0,  "== TX MODE ==", OLED_8X16);
  OLED_ShowString(0, 16, "S:0000 F:0000", OLED_8X16);
  OLED_ShowString(0, 32, "D:00 00 00 00", OLED_8X16);
  OLED_ShowString(0, 48, "Sending...", OLED_8X16);
  tx_data[0] = 0; tx_data[1] = 0;
  tx_data[2] = 0; tx_data[3] = 0;
  send_ok = 0; send_fail = 0;
  last_send_ms = HAL_GetTick();
#else
  OLED_ShowString(0, 0,  "== RX MODE ==", OLED_8X16);
  OLED_ShowString(0, 16, "R:0000 E:0000", OLED_8X16);
  OLED_ShowString(0, 32, "D:00 00 00 00", OLED_8X16);
  OLED_ShowString(0, 48, "Listening...", OLED_8X16);
  recv_ok = 0; recv_fail = 0;
#endif
  OLED_Update();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

#if DEVICE_MODE == MODE_TX
    /* ===== 发送端：每500ms自动发一包 ===== */
    if(HAL_GetTick() - last_send_ms >= 500)
    {
      last_send_ms = HAL_GetTick();

      /* 数据递增，RX端能看到变化 */
      tx_data[0]++;
      if(tx_data[0] == 0) tx_data[1]++;
      if(tx_data[0] == 0 && tx_data[1] == 0) tx_data[2]++;
      if(tx_data[0] == 0 && tx_data[1] == 0 && tx_data[2] == 0) tx_data[3]++;

      /* 装填并发送 */
      NRF24L01_TxPacket[0] = tx_data[0];
      NRF24L01_TxPacket[1] = tx_data[1];
      NRF24L01_TxPacket[2] = tx_data[2];
      NRF24L01_TxPacket[3] = tx_data[3];
      send_flag = NRF24L01_Send();

      if(send_flag == 1) send_ok++;
      else               send_fail++;

      /* 刷新数据行（先不画"TX"，避免闪两下） */
      // 第2行 y=16：S:xxxx F:xxxx
      OLED_ShowString(0, 16, "S:", OLED_8X16);
      OLED_ShowNum(2*8, 16, send_ok, 4, OLED_8X16);
      OLED_ShowString(6*8, 16, "F:", OLED_8X16);
      OLED_ShowNum(8*8, 16, send_fail, 4, OLED_8X16);
      
      // 第3行 y=32：D:xx xx xx xx
      OLED_ShowString(0, 32, "D:", OLED_8X16);
      OLED_ShowHexNum(2*8, 32, tx_data[0], 2, OLED_8X16);
      OLED_ShowHexNum(5*8, 32, tx_data[1], 2, OLED_8X16);
      OLED_ShowHexNum(8*8, 32, tx_data[2], 2, OLED_8X16);
      OLED_ShowHexNum(11*8, 32, tx_data[3], 2, OLED_8X16);
      
      // 第4行 y=48：TX闪烁标记
      OLED_ShowString(0, 48, "TX", OLED_8X16);
      OLED_Update();

      /* 局部擦除TX标记，实现闪烁 */
      OLED_ShowString(0, 48, "  ", OLED_8X16);
      OLED_UpdateArea(0, 48, 16, 16); // 参数：x起点, y起点, 宽度, 高度
    }
#endif

#if DEVICE_MODE == MODE_RX
    /* ===== 接收端：持续监听 ===== */
    recv_flag = NRF24L01_Receive();
    if(recv_flag)
    {
      if(recv_flag == 1)
      {
        recv_ok++;
        rx_data[0] = NRF24L01_RxPacket[0];
        rx_data[1] = NRF24L01_RxPacket[1];
        rx_data[2] = NRF24L01_RxPacket[2];
        rx_data[3] = NRF24L01_RxPacket[3];
      }
      else
      {
        recv_fail++;
      }

      /* 刷新数据行 + 显示"RX" */
     // 第2行 y=16：R:xxxx E:xxxx
      OLED_ShowString(0, 16, "R:", OLED_8X16);
      OLED_ShowNum(2*8, 16, recv_ok, 4, OLED_8X16);
      OLED_ShowString(6*8, 16, "E:", OLED_8X16);
      OLED_ShowNum(8*8, 16, recv_fail, 4, OLED_8X16);
      
      // 第3行 y=32：D:xx xx xx xx
      OLED_ShowString(0, 32, "D:", OLED_8X16);
      OLED_ShowHexNum(2*8, 32, rx_data[0], 2, OLED_8X16);
      OLED_ShowHexNum(5*8, 32, rx_data[1], 2, OLED_8X16);
      OLED_ShowHexNum(8*8, 32, rx_data[2], 2, OLED_8X16);
      OLED_ShowHexNum(11*8, 32, rx_data[3], 2, OLED_8X16);
      
      // 第4行 y=48：RX闪烁标记
      OLED_ShowString(0, 48, "RX", OLED_8X16);
      OLED_Update();

      /* 局部擦除RX标记 */
      OLED_ShowString(0, 48, "  ", OLED_8X16);
      OLED_UpdateArea(0, 48, 16, 16);
    }
#endif

  }
  /* USER CODE END 3 */
}

/* ==================== CubeMX生成的外设初始化 ==================== */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    Error_Handler();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    Error_Handler();
}

static void MX_I2C2_Init(void)
{
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x40B285C2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
    Error_Handler();
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
    Error_Handler();
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
    Error_Handler();
}

static void MX_SPI1_Init(void)
{
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
    Error_Handler();
}

static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
    Error_Handler();
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
    Error_Handler();
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
    Error_Handler();
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
    Error_Handler();
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(CE_GPIO_Port, CE_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(CSN_GPIO_Port, CSN_Pin, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = CE_Pin|CSN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1) { }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) { }
#endif /* USE_FULL_ASSERT */
