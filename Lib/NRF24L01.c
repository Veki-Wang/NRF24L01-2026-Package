#include "main.h"
#include "NRF24L01_Define.h"
#include "NRF24L01.h"
 
/* 发送部分全局变量 */
uint8_t NRF24L01_TxAddress[5] = {0x11, 0x22, 0x33, 0x44, 0x55};	//发送地址，固定5字节
#define NRF24L01_TX_PACKET_WIDTH		4	//发送数据包宽度，范围：1~32字节
uint8_t NRF24L01_TxPacket[NRF24L01_TX_PACKET_WIDTH];		//发送数据包
/* 接收部分全局变量 */
uint8_t NRF24L01_RxAddress[5] = {0x11, 0x22, 0x33, 0x44, 0x55};	//接收通道0地址，固定5字节
#define NRF24L01_RX_PACKET_WIDTH		4	//接收通道0数据包宽度，范围：1~32字节
uint8_t NRF24L01_RxPacket[NRF24L01_RX_PACKET_WIDTH];		//接收数据包

/* 引脚配置部分 */
void NRF24L01_W_CE(uint8_t BitValue)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, (GPIO_PinState)BitValue);
	//置PB0引脚（连接CE）为高/低电平，高or低取决于函数参数值
}

void NRF24L01_W_CSN(uint8_t BitValue)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, (GPIO_PinState)BitValue);
	//置PB1引脚（连接CSN）为高/低电平，高or低取决于函数参数值
}

void NRF24L01_W_SCK(uint8_t BitValue)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, (GPIO_PinState)BitValue);
	//置PA5引脚（连接SCK）为高/低电平，高or低取决于函数参数值
}

void NRF24L01_W_MOSI(uint8_t BitValue)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, (GPIO_PinState)BitValue);
	//置PA7引脚（连接MOSI）为高/低电平，高or低取决于函数参数值
}

uint8_t NRF24L01_R_MISO(void)
{
	return (uint8_t)HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6);
	//读取PA6引脚（连接MISO）的电平并返回
}

/* SPI通信实现部分 */
uint8_t NRF24L01_SPI_SwapByte(uint8_t Byte)  //一次字节数据交换
{
	//主机把要发送的Byte从最高位开始逐位移到MOSI引脚输出
	//同时将从机在MISO引脚返回的数据从最高位开始逐位从Byte的低位移入
	for(int i = 0; i < 8; i++)
	{
		//从高位移出数据
		if(Byte & 0x80)
		{
			NRF24L01_W_MOSI(1);
		}
		else
		{
			NRF24L01_W_MOSI(0);
		}
		
		Byte <<= 1;   //Byte左移，高位移出，低位补0
		
		NRF24L01_W_SCK(1);  //SCK置高电平
		
		//移入数据至低位
		if(NRF24L01_R_MISO())
		{
			Byte |= 0x01;
		}
		else
		{
			Byte &= ~0x01;
		}
		
		NRF24L01_W_SCK(0);  //SCK置低电平
	}
	
	return Byte;
}

/* 指令时序实现部分 */
void NRF24L01_WriteReg(uint8_t RegAddress, uint8_t Data)
{
	//在指定（单字节）寄存器地址RegAddress中写入Data
	NRF24L01_W_CSN(0);            //SPI通信起始
	NRF24L01_SPI_SwapByte(NRF24L01_W_REGISTER | RegAddress);  //发送写指令码，同时发送寄存器地址
	NRF24L01_SPI_SwapByte(Data);  //发送写入寄存器的数据
	NRF24L01_W_CSN(1);            //SPI通信结束
}
 
uint8_t NRF24L01_ReadReg(uint8_t RegAddress)
{
	//读取指定地址RegAddress（单字节）寄存器中的数据
	uint8_t Data;
	NRF24L01_W_CSN(0);            //SPI通信起始
	NRF24L01_SPI_SwapByte(NRF24L01_R_REGISTER | RegAddress);  //发送读指令码，同时发送寄存器地址
	Data = NRF24L01_SPI_SwapByte(NRF24L01_NOP);  //接收寄存器中的数据,用空指令做交换
	NRF24L01_W_CSN(1);            //SPI通信结束
	
	return Data;
}
 
void NRF24L01_WriteRegs(uint8_t RegAddress, uint8_t* DataArray, uint8_t Count)
{
	//在指定（多字节）寄存器地址RegAddress中写入Data
	NRF24L01_W_CSN(0);            //SPI通信起始
	NRF24L01_SPI_SwapByte(NRF24L01_W_REGISTER | RegAddress);  //发送写指令码，同时发送寄存器地址
	for(int i  = 0; i < Count; i++)
	{
		NRF24L01_SPI_SwapByte(DataArray[i]);  //发送写入寄存器的1字节数据，寄存器有几个字节就发几次
	}
	NRF24L01_W_CSN(1);            //SPI通信结束
}
 
void NRF24L01_ReadRegs(uint8_t RegAddress, uint8_t* DataArray, uint8_t Count)
{
	//读取指定地址RegAddress（多字节）寄存器中的数据
	uint8_t Data;
	NRF24L01_W_CSN(0);            //SPI通信起始
	NRF24L01_SPI_SwapByte(NRF24L01_R_REGISTER | RegAddress);  //发送读指令码，同时发送寄存器地址
	for(int i  = 0; i < Count; i++)
	{
		DataArray[i] = NRF24L01_SPI_SwapByte(NRF24L01_NOP);  //接收寄存器中的1字节数据，寄存器有几个字节就收几次
	}
	NRF24L01_W_CSN(1);            //SPI通信结束
}
 
void NRF24L01_WriteTxPayload(uint8_t* DataArray, uint8_t Count)
{
	//写发送有效载荷至TX FIFO
	NRF24L01_W_CSN(0);            //SPI通信起始
	NRF24L01_SPI_SwapByte(NRF24L01_W_TX_PAYLOAD);  //发送写FIFO指令码
	for(int i  = 0; i < Count; i++)
	{
		NRF24L01_SPI_SwapByte(DataArray[i]);  //发送写入TX FIFO的1字节数据，数据段有几个字节就发几次
	}
	NRF24L01_W_CSN(1);            //SPI通信结束
}
 
void NRF24L01_ReadRxPayload(uint8_t* DataArray, uint8_t Count)
{
	//从RX FIFO中读取有效载荷
	NRF24L01_W_CSN(0);            //SPI通信起始
	NRF24L01_SPI_SwapByte(NRF24L01_R_RX_PAYLOAD);  //发送读FIFO指令码
	for(int i  = 0; i < Count; i++)
	{
		DataArray[i] = NRF24L01_SPI_SwapByte(NRF24L01_NOP);  //接收RX FIFO中的1字节数据，数据段有几个字节就收几次
	}
	NRF24L01_W_CSN(1);            //SPI通信结束
}
 
void NRF24L01_FlushTx(void)
{
	//清空TX FIFO
	NRF24L01_W_CSN(0);            //SPI通信起始
	NRF24L01_SPI_SwapByte(NRF24L01_FLUSH_TX);  //发送清空TX FIFO指令码
	NRF24L01_W_CSN(1);            //SPI通信结束
}
 
void NRF24L01_FlushRx(void)
{
	//清空RX FIFO
	NRF24L01_W_CSN(0);            //SPI通信起始
	NRF24L01_SPI_SwapByte(NRF24L01_FLUSH_RX);  //发送清空RX FIFO指令码
	NRF24L01_W_CSN(1);            //SPI通信结束
}
 
uint8_t NRF24L01_ReadStatus(void)
{
	//读取状态寄存器
	uint8_t Status;
	NRF24L01_W_CSN(0);            //SPI通信起始
	Status = NRF24L01_SPI_SwapByte(NRF24L01_NOP); //发送空指令，接收状态寄存器的值
	NRF24L01_W_CSN(1);            //SPI通信结束
	
	return Status;
}

void NRF24L01_PowerDown(void)  //控制NRF24L01进入掉电模式
{
	uint8_t Config;
	/* CE置为0 */
	NRF24L01_W_CE(0);
	
	/* PWR_UP置为0 */
	Config = NRF24L01_ReadReg(NRF24L01_CONFIG); //获取配置寄存器的值
	if (Config == 0xFF) {return;}					//配置寄存器全为1，出错，退出函数
	Config &= ~0x02;
	NRF24L01_WriteReg(NRF24L01_CONFIG, Config); //置配置寄存器第1位为0，其它位保持原值
}
 
void NRF24L01_StandbyI(void)  //控制NRF24L01进入待机模式1
{
	uint8_t Config;
	
	/* CE置为0 */
	NRF24L01_W_CE(0);
 
	/* PWR_UP置为1 */
	Config = NRF24L01_ReadReg(NRF24L01_CONFIG); //获取配置寄存器的值
	if (Config == 0xFF) {return;}					//配置寄存器全为1，出错，退出函数
	Config |= 0x02;
	NRF24L01_WriteReg(NRF24L01_CONFIG, Config); //置配置寄存器第1位为1，其它位保持原值
}
 
void NRF24L01_RxMode(void)  //控制NRF24L01进入接收模式
{
	uint8_t Config;
	/* CE置为0 */
	NRF24L01_W_CE(0);
	
	/* PWR_UP置为1，PRIM_RX置为1 */
	Config = NRF24L01_ReadReg(NRF24L01_CONFIG); //获取配置寄存器的值
	if (Config == 0xFF) {return;}					//配置寄存器全为1，出错，退出函数
	Config |= 0x03;
	NRF24L01_WriteReg(NRF24L01_CONFIG, Config); //置配置寄存器第0位和第1位为1，其它位保持原值
	
	/* CE置为1 */
	NRF24L01_W_CE(1);
}
 
void NRF24L01_TxMode(void)  //控制NRF24L01进入发送模式
{
	uint8_t Config;
	/* CE置为0 */
	NRF24L01_W_CE(0);
	
	/* PWR_UP置为1，PRIM_RX置为0 */
	Config = NRF24L01_ReadReg(NRF24L01_CONFIG); //获取配置寄存器的值
	if (Config == 0xFF) {return;}					//配置寄存器全为1，出错，退出函数
	Config |= 0x02;
	Config &= ~0x01;
	NRF24L01_WriteReg(NRF24L01_CONFIG, Config); //置配置寄存器第0位为0、第1位为1，其它位保持原值
	
	/* CE置为1 */
	NRF24L01_W_CE(1);
}

void NRF24L01_Init(void)
{
	/* ----- 引脚GPIO初始化（软件SPI：PA5=SCK, PA6=MISO, PA7=MOSI）----- */
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/* PA5(SCK) 和 PA7(MOSI) → 推挽输出 */
	GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* PA6(MISO) → 输入 */
	GPIO_InitStruct.Pin = GPIO_PIN_6;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* 初始电平：SCK=0, MOSI=0 */
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);

	/* ----- 寄存器初始化，配置初始参数 ----- */
	NRF24L01_WriteReg(NRF24L01_CONFIG, 0x08);      //CONFIG寄存器初始化
	NRF24L01_WriteReg(NRF24L01_EN_AA, 0x3F);       //EN_AA寄存器初始化
	NRF24L01_WriteReg(NRF24L01_EN_RXADDR, 0x01);   //EN_RXADDR寄存器初始化，只开启接收通道0
	NRF24L01_WriteReg(NRF24L01_SETUP_AW, 0x03);    //SETUP_AW寄存器初始化，配置地址宽度为5
	NRF24L01_WriteReg(NRF24L01_SETUP_RETR, 0x03);  //SETUP_RETR寄存器初始化，配置地址宽度为5
	NRF24L01_WriteReg(NRF24L01_RF_CH, 0x02);   //RF_CH寄存器初始化，配置工作频率
	NRF24L01_WriteReg(NRF24L01_RF_SETUP, 0x0E);    //RF_SETUP寄存器初始化

	NRF24L01_WriteRegs(NRF24L01_RX_ADDR_P0, NRF24L01_RxAddress, 5);    //接收通道0地址配置

	NRF24L01_WriteReg(NRF24L01_RX_PW_P0, NRF24L01_RX_PACKET_WIDTH);    //NRF24L01_RX_PW_P0寄存器初始化，配置接收通道0接收数据包宽度

	/* 默认进入接收模式 */
	NRF24L01_RxMode();
}

uint8_t NRF24L01_Send(void)
{
	uint8_t Status;
	uint8_t SendFlag;
	uint32_t Timeout;
	
	/* 写入发送通道地址 */
	NRF24L01_WriteRegs(NRF24L01_TX_ADDR, NRF24L01_TxAddress, 5);
	
	/* 写入发送数据 */
	NRF24L01_WriteTxPayload(NRF24L01_TxPacket, NRF24L01_TX_PACKET_WIDTH);
	
	/* 写入接收通道0的地址（开启自动应答后，需与发送通道地址一致，以接收应答包） */
	NRF24L01_WriteRegs(NRF24L01_RX_ADDR_P0, NRF24L01_TxAddress, 5);
	/* 进入发送模式 */
	NRF24L01_TxMode();
	
	/* 指定超时时间，即循环读取状态寄存器的次数 */
	Timeout = 10000;
	
	while(1)
	{
		/* 读取状态寄存器 */
		Status = NRF24L01_ReadStatus();
		
		Timeout --;                 //超时计次
		if(Timeout == 0)
		{
			SendFlag = 4;           //认为发送超时，置标志位为4
			NRF24L01_Init();        //重新初始化设备
			break;                //跳出循环
		}
		
		/* 根据状态寄存器的值，判断发送状态 */
		if((Status & 0x30) == 0x30)    //MAX_RT和TX_DS同时为1
		{
			SendFlag = 3;           //认为状态寄存器的值不合法，置标志位为3
			NRF24L01_Init();        //重新初始化设备
			break;                //跳出循环
		}
		else if((Status & 0x10) == 0x10)  //MAX_RT为1
		{
			SendFlag = 2;           //达到了最大重发次数仍未收到应答，置标志位为2
			NRF24L01_Init();        //重新初始化设备
			break;                //跳出循环
		}
		else if((Status & 0x20) == 0x20)  //TX_DS为1
		{
			SendFlag = 1;            //发送成功，无错误，置标志位为1
			break;                  //跳出循环
		}
	}
	
	/* 给状态寄存器的位4（MAX_RT）和位5（TX_DS）写1，清标志位 */
	NRF24L01_WriteReg(NRF24L01_STATUS, 0x30);
 
	/* 清空Tx FIFO的所有数据 */
	NRF24L01_FlushTx();
 
	/* 发送完成后，恢复接收通道0原来的地址 */
	//如果发送地址和接收通道0地址设置相同，则可不执行这一句
	NRF24L01_WriteRegs(NRF24L01_RX_ADDR_P0, NRF24L01_RxAddress, 5);
	
	/* 发送完成，芯片恢复为接收模式 */
	NRF24L01_RxMode();
	
	/* 返回发送标志位 */
	return SendFlag;
}
uint8_t NRF24L01_Receive(void)
{
	uint8_t Status, Config;
	uint8_t ReceiveFlag;
	
	/* 读取状态寄存器 */
	Status = NRF24L01_ReadStatus();
	
	/* 读取配置寄存器 */
	Config = NRF24L01_ReadReg(NRF24L01_CONFIG);
	
	/* 根据配置寄存器和状态寄存器的值，判断接收状态 */
	if ((Config & 0x02) == 0x00)            //PWR_UP为0
	{
		ReceiveFlag = 3;                 //设备仍处于掉电模式，置标志位为3
		NRF24L01_Init();                //重新初始化设备
	}
	else if ((Status & 0x30) == 0x30)        //MAX_RT和TX_DS同时为1
	{
		ReceiveFlag = 2;                 //认为状态寄存器的值不合法，置标志位为2
		NRF24L01_Init();                //重新初始化设备
	}
	else if ((Status & 0x40) == 0x40)   	  //RX_DR为1
	{
		ReceiveFlag = 1;                 //接收到数据，置标志位为1
		
		/* 读接收有效载荷 */
		NRF24L01_ReadRxPayload(NRF24L01_RxPacket, NRF24L01_RX_PACKET_WIDTH);
		
		/* 给状态寄存器的位6（RX_DR）写1，清标志位 */
		NRF24L01_WriteReg(NRF24L01_STATUS, 0x40);
 
		/* 清空Rx FIFO的所有数据 */
		NRF24L01_FlushRx();
	}
	else
	{
		ReceiveFlag = 0;				//未接收到数据，置标志位为0
	}
	
	/* 返回接收标志位 */
	return ReceiveFlag;
}

