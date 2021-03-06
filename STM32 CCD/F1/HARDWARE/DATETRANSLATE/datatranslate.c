#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "datatranslate.h"
#include "CCD.h"
#include "road_identify.h"

extern  unsigned char image[row][line];
extern int left[row];             //左边界数组
extern int right[row];            //右边界数组
extern int bottom[row];           //中线数�
extern int ph;
/*校验和发送数据函数_______________________*/
void Uart_PrintfPhoto(void);
//****************************************************************************
//  函数名：CRC_CHECK(unsigned char *Buf, unsigned char CRC_CNT)
//  功能：校验函数
//  说明：无
//*****************************************************************************/ 
unsigned short CRC_CHECK(unsigned char *Buf, unsigned char CRC_CNT)
{
    unsigned short CRC_Temp;
    unsigned char i,j;
    CRC_Temp = 0xffff;

    for (i=0;i<CRC_CNT; i++)
	{      
        CRC_Temp ^= Buf[i];
        for (j=0;j<8;j++) 
		{
            if (CRC_Temp & 0x01)
                CRC_Temp = (CRC_Temp >>1 ) ^ 0xa001;
            else
                CRC_Temp = CRC_Temp >> 1;
        }
    }
    return(CRC_Temp);
}

//****************************************************************************
//  函数名：OutPut_Data(float a,float b,float c,float d)
//  功能：发送数据函数
//  说明：无
//*****************************************************************************/ 
void OutPut_Data(float a,float b,float c,float d)
{	
	float OutData[4] = {0};
	int temp[4] = {0};
	unsigned int temp1[4] = {0};
	unsigned char databuf[10] = {0};
	unsigned char i;
	unsigned short CRC16 = 0;
	OutData[0]=a;
	OutData[1]=b;
	OutData[2]=c;
	OutData[3]=d;
	for(i=0;i<4;i++)
	{
		temp[i]  = (int)OutData[i];
		temp1[i] = (unsigned int)temp[i];
	}   
	for(i=0;i<4;i++) 
	{
		databuf[i*2]   = (unsigned char)(temp1[i]%256);
		databuf[i*2+1] = (unsigned char)(temp1[i]/256);
	}
  
	CRC16 = CRC_CHECK(databuf,8);
	databuf[8] = CRC16%256;
	databuf[9] = CRC16/256;
  
	for(i=0;i<10;i++) 
	{
		USART_SendData(USART1, databuf[i]);//发送函数(串口1)
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
	}     
}

/*无_______________________*/

//****************************************************************************
//  函数名：Output_Ccd(unsigned char CCD_Value[])
//  功能：无
//  说明：无
//*****************************************************************************/ 
void Output_Ccd(unsigned char CCD_Value[])
{
	int i;
    USART_SendData(USART1, 0xff);
    USART_SendData(USART1, 0x00);

	for(i=0;i<128;i++)
	{
		if(CCD_Value[i]>254) CCD_Value[i]=254;
		if(CCD_Value[i]<1) CCD_Value[i]=1;
		USART_SendData(USART1, CCD_Value[i]);
	}
}

//****************************************************************************
//  函数名：StoDChange(char dat[],char num)
//  功能：无
//  说明：无
//*****************************************************************************/ 
float StoDChange(char dat[],char num)
{
	//num 4
	char i,sign=0;
	float fdata;
    if(dat[4]=='-') 
		sign=1;
    else 
		sign=0;
	for(i=num;i<10;i++)
	{
		if(dat[i]>='0' && dat[i]<='9') 
			dat[i]=dat[i]-'0';
		else if(dat[i]>='A' && dat[i]<='F') 
			dat[i]=dat[i]-'A';
		else if(dat[i]=='\0') 
			break;
		else 
			dat[i]=0;
	}   
	fdata=dat[num]*100+dat[num+1]*10+dat[num+2]+dat[num+3]/10.0+dat[num+4]/100.0; 
	if(sign==1) 
		fdata=-fdata; 
	return fdata; 
}

/*红树伟业上位换�(发送图像)______________________*/

//****************************************************************************
//  函数名：Uart_PrintfPhoto(void)
//  功能：通过蓝牙向上位机发送二值化后图像
//  说明：无
//*****************************************************************************/ 

void Uart_PrintfPhoto(void)
{
	int i,j;
	for(i=0;i<row;i++)
	{
		for(j=0;j<=line-1;j++)
		{
//			if(image[i][j]==0x00)
//			USART_SendData(USART1,0x00);
//			else
//			USART_SendData(USART1,0xfe);
				USART_SendData(USART1,image[i][j]);
				while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
		}				
	}
     USART_SendData(USART1,0xff);
}

//****************************************************************************
//  函数名：Uart_PrintfPhoto1(void)
//  功能：通过蓝牙向上位机发送二值图及其中线位置
//  说明：左右边界发送0xA0,赛道中心发送0xfe，前瞻行底色浅一些
//*****************************************************************************/ 
void Uart_PrintfPhoto1(void)
{
	static   int i,j;
		for(i=row-1;i>=0;i--)
		{
			for(j=0;j<=line-1;j++)
			{
				if(i==ph)
				{
					if((j==left[i])||(j==right[i]))	{	
				
						USART_SendData(USART1,0x00);
					if (j==bottom[i]){
						 USART_SendData(USART1,0xfe);
					 }
					
					}
					else
					{
						if(image[i][j]==0x10)	
							USART_SendData(USART1,0x20);
						else
							USART_SendData(USART1,0xe0);
					}
				}
				else
				{
					if((j==left[i])||(j==right[i]))	{
						USART_SendData(USART1,0xa0);
				   	if (j==bottom[i]){
						 USART_SendData(USART1,0xfe);
					 }
					}
					else
					{
						if(image[i][j]==0x01)	
							USART_SendData(USART1,0x00);
						else
							USART_SendData(USART1,0xfe);
					}
				}
			}				
		}
		USART_SendData(USART1,0xff);
}























