/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "NRF24L01.h"
#include "Key.h"
#include "OLED.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define KEY1  1
#define KEY2  2
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

uint8_t KeyNum;
 
uint8_t SendFlag;								//发送标志位
uint8_t SendSuccessCount, SendFailedCount;		//发送成功计次，发送失败计次
 
uint8_t ReceiveFlag;							//接收标志位
uint8_t ReceiveSuccessCount, ReceiveFailedCount;   //接收成功计次，接收失败计次
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

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */

  /* ====== 第一步：LED闪烁3次，验证代码在运行 ====== */
  /* 用PB0(CE引脚)作为调试灯，闪烁3次 */
  for(int i = 0; i < 3; i++)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_Delay(200);
  }
  /* 如果LED没有闪烁 → MCU没运行，检查供电和烧录 */

  /* ====== 第二步：初始化OLED ====== */
  OLED_Init();
  OLED_Clear();
  OLED_ShowString(0, 0, "T:000-000-0", OLED_8X16);
  OLED_ShowString(0, 16,"R:000-000-0", OLED_8X16);
  OLED_Update();

  /* ====== 第三步：I2C自检 ====== */
  {
    /* 尝试读取OLED状态（发送一个命令后检查I2C是否ACK） */
    uint8_t test_cmd = 0x00;
    HAL_StatusTypeDef i2c_status;
    i2c_status = HAL_I2C_Mem_Write(&hi2c2, 0x78, 0x00,
                                    I2C_MEMADD_SIZE_8BIT, &test_cmd, 1, 100);
    if(i2c_status != HAL_OK)
    {
      /* I2C通信失败！可能是地址不对或接线问题 */
      /* LED快闪10次表示I2C错误 */
      for(int i = 0; i < 10; i++)
      {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
        HAL_Delay(50);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_Delay(50);
      }
      /* 尝试备用地址 0x7A（部分OLED模块SA0引脚接高电平） */
      i2c_status = HAL_I2C_Mem_Write(&hi2c2, 0x7A, 0x00,
                                      I2C_MEMADD_SIZE_8BIT, &test_cmd, 1, 100);
      if(i2c_status == HAL_OK)
      {
        /* 地址是0x7A，需要修改OLED.c中的地址 */
        /* LED慢闪2次表示找到了备用地址 */
        for(int i = 0; i < 2; i++)
        {
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
          HAL_Delay(500);
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
          HAL_Delay(500);
        }
        /* 卡住：请修改OLED.c中0x78为0x7A */
        while(1);
      }
      else
      {
        /* 两个地址都不通 → 检查接线 */
        while(1);  // LED灭=硬件问题
      }
    }
  }

  /* ====== 第四步：NRF24L01自检 ====== */
  NRF24L01_Init();
  {
    uint8_t check_val = NRF24L01_ReadReg(NRF24L01_CONFIG);
    if(check_val == 0xFF || check_val == 0x00)
    {
      OLED_Clear();
      OLED_ShowString(0, 0, "NRF ERR!", OLED_8X16);
      OLED_Update();
      /* LED常亮 = NRF24L01异常 */
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
      while(1);
    }
    else
    {
      OLED_Clear();
      OLED_ShowString(0, 0, "NRF OK!", OLED_8X16);
      OLED_Update();
      HAL_Delay(500);
      OLED_ShowString(0, 0, "T:000-000-0", OLED_8X16);
      OLED_ShowString(0, 16,"R:000-000-0", OLED_8X16);
      OLED_Update();
    }
  }

  /* 初始化发送数据 */
  NRF24L01_TxPacket[0] = 0x00;
  NRF24L01_TxPacket[1] = 0x01;
  NRF24L01_TxPacket[2] = 0x02;
  NRF24L01_TxPacket[3] = 0x03;

  /* 初始化计数变量 */
  SendSuccessCount = 0;
  SendFailedCount = 0;
  ReceiveSuccessCount = 0;
  ReceiveFailedCount = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    KeyNum = Key_Get();		//读取按键，获取键码
		
		if (KeyNum == 1)				//按键按下
		{
			/* 变换测试数据，便于观察实验现象 */
			NRF24L01_TxPacket[0]++;
			NRF24L01_TxPacket[1]++;
			NRF24L01_TxPacket[2]++;
			NRF24L01_TxPacket[3]++;
			
			/* 调用NRF24L01_Send函数，发送数据，同时返回发送标志位 */
			SendFlag = NRF24L01_Send();
			
			if (SendFlag == 1){			//发送标志位为1，表示发送成功
				SendSuccessCount++;	//发送成功计次变量自增
			}
			else{						//发送标志位不为1，即2/3/4，表示发送不成功
				SendFailedCount++;	//发送失败计次变量自增
			}
			
			OLED_ShowNum(1, 3, SendSuccessCount, 3, OLED_8X16);	//显示发送成功次数
			OLED_ShowNum(1, 7, SendFailedCount, 3, OLED_8X16);		//显示发送失败次数
			OLED_ShowNum(1, 11, SendFlag, 1, OLED_8X16);			//显示最近一次的发送标志位

			/* OLED显示发送数据 */
			OLED_ShowHexNum(2, 1, NRF24L01_TxPacket[0], 2, OLED_8X16);
			OLED_ShowHexNum(2, 4, NRF24L01_TxPacket[1], 2, OLED_8X16);
			OLED_ShowHexNum(2, 7, NRF24L01_TxPacket[2], 2, OLED_8X16);
			OLED_ShowHexNum(2, 10, NRF24L01_TxPacket[3], 2, OLED_8X16);

			/* TX字符串闪烁一次，表明发送了一次数据 */
			OLED_ShowString(1, 15, "TX", OLED_8X16);
			OLED_Update();
			HAL_Delay(100);
			OLED_ShowString(1, 15, "  ", OLED_8X16);
			OLED_Update();
		}
		
		/* 调用NRF24L01_Receive函数，接收数据，同时返回接收标志位 */
		ReceiveFlag = NRF24L01_Receive();
		
		if (ReceiveFlag)				//接收标志位不为0，表示收到了一个数据包
		{
			if (ReceiveFlag == 1){		//接收标志位为1，表示接收成功
				ReceiveSuccessCount++;	//接收成功计次变量自增
			}
			else{		//接收标志位不为0也不为1，即2/3，表示此次接收产生了错误
				ReceiveFailedCount++;	//接收失败计次变量自增
			}
			
			OLED_ShowNum(3, 3, ReceiveSuccessCount, 3, OLED_8X16);	//显示接收成功次数
			OLED_ShowNum(3, 7, ReceiveFailedCount, 3, OLED_8X16);	//显示接收失败次数
			OLED_ShowNum(3, 11, ReceiveFlag, 1, OLED_8X16);		//显示最近一次的接收标志位

			/* OLED显示接收数据 */
			OLED_ShowHexNum(4, 1, NRF24L01_RxPacket[0], 2, OLED_8X16);
			OLED_ShowHexNum(4, 4, NRF24L01_RxPacket[1], 2, OLED_8X16);
			OLED_ShowHexNum(4, 7, NRF24L01_RxPacket[2], 2, OLED_8X16);
			OLED_ShowHexNum(4, 10, NRF24L01_RxPacket[3], 2, OLED_8X16);

			/* RX字符串闪烁一次，表明接收到了一次数据 */
			OLED_ShowString(3, 15, "RX", OLED_8X16);
			OLED_Update();
			HAL_Delay(100);
			OLED_ShowString(3, 15, "  ", OLED_8X16);
			OLED_Update();

    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
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
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
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
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

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
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CE_GPIO_Port, CE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CSN_GPIO_Port, CSN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : CE_Pin CSN_Pin */
  GPIO_InitStruct.Pin = CE_Pin|CSN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : key1_Pin key2_Pin */
  GPIO_InitStruct.Pin = key1_Pin|key2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
