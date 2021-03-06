#include "uart.h"
#define MAX_SIZE 100
#define UART_REC_LEN 125
u32 BaudRate = 9600;   //115200;
u8 RX_DATA_BUFF[125];
u8 UART_RX_BUF[256];
u8 len = 0;
u8 t;
u16 UART_RX_STA = 0;
extern u8 txBuf[MAX_SIZE], rxBuf[MAX_SIZE], txLen, PosW;
extern u16 RxCont;
extern unsigned int RxTimeout, TxTimeout, SysTick_Count;
/********************************************************************************************************
 **function: Usart2SendByte
 **@brief    Data sending API functions.Serial communication can call this function to send data//TEST
 **          This function is tested for use
 **@param    data : Data address
 **
 **@param    len : Data length
 **
 **@return   None.
 ********************************************************************************************************/
void Usart2SendByte(char *data, u8 len) {
	u8 t = 0;
	for (t = 0; t < len; t++) {
		RX_DATA_BUFF[t] = data[t];
		while ((UART2->CSR & 0x1) == 0)
			; //Send data cyclically   while((UART1->SR&0X40)==0);//
		UART2->TDR = RX_DATA_BUFF[t];
	}
	UART_RX_STA = 0;
}

/********************************************************************************************************
 **function: uart_initwBaudRate
 **@brief    Use this function to configure the basic configuration of the serial port,
 **          such as serial clock, pins, interrupts
 **@param    None
 **
 **@return   None.
 ********************************************************************************************************/
void uart_initwBaudRate(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	UART_InitTypeDef UART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART2, ENABLE); //Enable UART2 clock
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);   //Enable GPIOA clock
	//UART2 NVIC Cfg
	NVIC_InitStructure.NVIC_IRQChannel = UART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPriority = 3;      //Preemption priority 3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;   //UART2 IRQ channel enable
	NVIC_Init(&NVIC_InitStructure); //Initialize the VIC register according to the specified parameters

	//UART  initialization Cfg
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_1);

	UART_InitStructure.UART_BaudRate = BaudRate;              //BaudRate
	UART_InitStructure.UART_WordLength = UART_WordLength_8b; //Word length is 8-bit data format
	UART_InitStructure.UART_StopBits = UART_StopBits_1;       //stop bit
	UART_InitStructure.UART_Parity = UART_Parity_No;          //No parity
	UART_InitStructure.UART_HardwareFlowControl = UART_HardwareFlowControl_None; //No hardware data flow control
	UART_InitStructure.UART_Mode = UART_Mode_Rx | UART_Mode_Tx; //Configure send and receive mode

	UART_Init(UART2, &UART_InitStructure); //uart2 enable
	UART_ITConfig(UART2, UART_IT_RXIEN, ENABLE); //Enable UART receive interrupt
	UART_ITConfig(UART2, UART_IT_TXIEN, DISABLE);
	UART_Cmd(UART2, ENABLE);

	//UART2_TX   GPIOA.2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;         //PA2
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //Multiplexed push-pull output
	GPIO_Init(GPIOA, &GPIO_InitStructure);            //GPIO initialization

	//UART2_RX   GPIOA.3
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;             //PA3
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //Floating input
	GPIO_Init(GPIOA, &GPIO_InitStructure);                //GPIO initialization
}

/********************************************************************************************************
 **function: ChangeBaudRate
 **@brief    Use this function to configure BaudRate
 **
 **@param    None.
 **
 **@return   None.
 ********************************************************************************************************/
void ChangeBaudRate(void) {
	uint32_t apbclock = 0x00;
	RCC_ClocksTypeDef RCC_ClocksStatus;
	RCC_GetClocksFreq(&RCC_ClocksStatus);
	UART_Cmd(UART2, DISABLE);
	apbclock = RCC_ClocksStatus.PCLK1_Frequency;
	UART2->BRR = (apbclock / BaudRate) / 16;
	UART2->FRA = (apbclock / BaudRate) % 16;
	UART_Cmd(UART1, ENABLE);
}

/********************************************************************************************************
 **function: moduleOutData
 **@brief    Data sending API functions.Serial communication can call this function to send data
 **
 **@param    data : Data address
 **
 **@param    len : Data length
 **
 **@return   None.
 ********************************************************************************************************/
void moduleOutData(u8 *data, u8 len) //api
{
	unsigned char i;
	if ((txLen + len) < MAX_SIZE) //buff not overflow
	{
		for (i = 0; i < len; i++) {
			txBuf[txLen + i] = *(data + i);
		}
		txLen += len;
	}
}

/********************************************************************************************************
 **function: UART2_IRQHandler
 **@brief    Serial port interrupt function.Secondary interrupts include receive interrupts
 **          and transmit interrupts. Data can be processed in this interrupt
 **@param    None.
 **
 **@return   None.
 ********************************************************************************************************/
void UART2_IRQHandler(void) {
	if (UART_GetITStatus(UART2, UART_IT_RXIEN) != RESET) {
		UART_ClearITPendingBit(UART2, UART_IT_RXIEN);
		rxBuf[RxCont] = UART_ReceiveData(UART2);
		RxTimeout = SysTick_Count + 1000;
		RxCont++;
		if (RxCont >= MAX_SIZE) {
			RxCont = 0;
		}
	}
	if (UART_GetITStatus(UART2, UART_IT_TXIEN) != RESET) {
		UART_ClearITPendingBit(UART2, UART_IT_TXIEN);
		TxTimeout = SysTick_Count + (20000 / BaudRate);
		if (PosW < txLen) {
			UART_SendData(UART2, txBuf[PosW++]);
			if (PosW == txLen) {
				txLen = 0;
				PosW = 0;
			}
		} else {
			UART_ITConfig(UART2, UART_IT_TXIEN, DISABLE);
		}
	}
}

void UartSendByte(u8 dat) {
	UART_SendData(UART2, dat);
	while (!UART_GetFlagStatus(UART2, UART_FLAG_TXEPT))
		;
}

/********************************************************************************************************
 **函数信息 ：void UartSendGroup(u8* buf,u16 len)
 **功能描述 ：UART发送数据
 **输入参数 ：buf,len
 **输出参数 ：
 **    备注 ：
 ********************************************************************************************************/
void UartSendGroup(u8 *buf, u16 len) {
	while (len--)
		UartSendByte(*buf++);
}

