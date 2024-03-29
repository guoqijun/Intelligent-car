#include "ov7670.h"
#include "ov7670cfg.h"
#include "exti.h"
#include "bsp_SysTick.h"
#include "bsp_usart1.h"			 
#include "sccb.h"	
#include "lcd.h"
#include "wifi_config.h"
#include "wifi_function.h"
#include "stdio.h"
#include "string.h"
#include "malloc.h"	   
	    
//初始化OV7670
//返回0:成功
//返回其他值:错误代码
u8 OV7670_Init(void)
{
	u8 temp;
	u16 i=0;	  
	//设置IO
	RCC->APB2ENR|=1<<2;		//先使能外设PORTA时钟
	RCC->APB2ENR|=1<<3;		//先使能外设PORTB时钟
 	RCC->APB2ENR|=1<<4;		//先使能外设PORTC时钟
  	RCC->APB2ENR|=1<<5;		//先使能外设PORTD时钟
	RCC->APB2ENR|=1<<8;		//先使能外设PORTG时钟	   
 
  	GPIOA->CRH&=0XFFFFFFF0; 	   
  	GPIOA->CRH|=0X00000008; 	//PA8 输入  
	GPIOA->ODR|=1<<8; 
   	GPIOB->CRL&=0XFFF00FFF; 	  
  	GPIOB->CRL|=0X00033000; 	//PB3/4 输出
	GPIOB->ODR|=3<<3; 	    
  	GPIOC->CRL=0X88888888; 		//PC0~7 输入    
	GPIOC->ODR|=0x00ff; 
   	GPIOD->CRL&=0XF0FFFFFF; 	//PD6 输出   
  	GPIOD->CRL|=0X03000000; 	 
 	GPIOD->ODR|=1<<6; 
   	GPIOG->CRH&=0X00FFFFFF; 	 
	GPIOG->CRH|=0X33000000;	    
	GPIOG->ODR=7<<14;      		//PG14/15  输出高 
 	JTAG_Set(1);
 	SCCB_Init();        		//初始化SCCB 的IO口	   	  
 	if(SCCB_WR_Reg(0x12,0x80))return 1;	//复位SCCB	  
	Delay_ms(50); 
	//读取产品型号
 	temp=SCCB_RD_Reg(0x0b);   
	if(temp!=0x73)return 2;  
 	temp=SCCB_RD_Reg(0x0a);   
	if(temp!=0x76)return 2;
	//初始化序列	  
	for(i=0;i<sizeof(ov7670_init_reg_tbl)/sizeof(ov7670_init_reg_tbl[0]);i++)
	{
	   	SCCB_WR_Reg(ov7670_init_reg_tbl[i][0],ov7670_init_reg_tbl[i][1]);
		delay_ms(2);
 	}
	EXTI8_Init();								//使能定时器捕获
	OV7670_Window_Set(10,174,240,320);			//设置窗口	  
  	OV7670_CS=0;
	TIM4_Int_Init(499,7199);					//Tout= ((arr+1)*(psc+1))/Tclk；50ms中断一次，检查能否刷新图片
   	return 0x00; 	//ok
} 
////////////////////////////////////////////////////////////////////////////
//OV7670功能设置
//白平衡设置
//0:自动
//1:太阳sunny
//2,阴天cloudy
//3,办公室office
//4,家里home
void OV7670_Light_Mode(u8 mode)
{
	u8 reg13val=0XE7;//默认就是设置为自动白平衡
	u8 reg01val=0;
	u8 reg02val=0;
	switch(mode)
	{
		case 1://sunny
			reg13val=0XE5;
			reg01val=0X5A;
			reg02val=0X5C;
			break;	
		case 2://cloudy
			reg13val=0XE5;
			reg01val=0X58;
			reg02val=0X60;
			break;	
		case 3://office
			reg13val=0XE5;
			reg01val=0X84;
			reg02val=0X4c;
			break;	
		case 4://home
			reg13val=0XE5;
			reg01val=0X96;
			reg02val=0X40;
			break;	
	}
	SCCB_WR_Reg(0X13,reg13val);//COM8设置 
	SCCB_WR_Reg(0X01,reg01val);//AWB蓝色通道增益 
	SCCB_WR_Reg(0X02,reg02val);//AWB红色通道增益 
}				  
//色度设置
//0:-2
//1:-1
//2,0
//3,1
//4,2
void OV7670_Color_Saturation(u8 sat)
{
	u8 reg4f5054val=0X80;//默认就是sat=2,即不调节色度的设置
 	u8 reg52val=0X22;
	u8 reg53val=0X5E;
 	switch(sat)
	{
		case 0://-2
			reg4f5054val=0X40;  	 
			reg52val=0X11;
			reg53val=0X2F;	 	 
			break;	
		case 1://-1
			reg4f5054val=0X66;	    
			reg52val=0X1B;
			reg53val=0X4B;	  
			break;	
		case 3://1
			reg4f5054val=0X99;	   
			reg52val=0X28;
			reg53val=0X71;	   
			break;	
		case 4://2
			reg4f5054val=0XC0;	   
			reg52val=0X33;
			reg53val=0X8D;	   
			break;	
	}
	SCCB_WR_Reg(0X4F,reg4f5054val);	//色彩矩阵系数1
	SCCB_WR_Reg(0X50,reg4f5054val);	//色彩矩阵系数2 
	SCCB_WR_Reg(0X51,0X00);			//色彩矩阵系数3  
	SCCB_WR_Reg(0X52,reg52val);		//色彩矩阵系数4 
	SCCB_WR_Reg(0X53,reg53val);		//色彩矩阵系数5 
	SCCB_WR_Reg(0X54,reg4f5054val);	//色彩矩阵系数6  
	SCCB_WR_Reg(0X58,0X9E);			//MTXS 
}
//亮度设置
//0:-2
//1:-1
//2,0
//3,1
//4,2
void OV7670_Brightness(u8 bright)
{
	u8 reg55val=0X00;//默认就是bright=2
  	switch(bright)
	{
		case 0://-2
			reg55val=0XB0;	 	 
			break;	
		case 1://-1
			reg55val=0X98;	 	 
			break;	
		case 3://1
			reg55val=0X18;	 	 
			break;	
		case 4://2
			reg55val=0X30;	 	 
			break;	
	}
	SCCB_WR_Reg(0X55,reg55val);	//亮度调节 
}
//对比度设置
//0:-2
//1:-1
//2,0
//3,1
//4,2
void OV7670_Contrast(u8 contrast)
{
	u8 reg56val=0X40;//默认就是contrast=2
  	switch(contrast)
	{
		case 0://-2
			reg56val=0X30;	 	 
			break;	
		case 1://-1
			reg56val=0X38;	 	 
			break;	
		case 3://1
			reg56val=0X50;	 	 
			break;	
		case 4://2
			reg56val=0X60;	 	 
			break;	
	}
	SCCB_WR_Reg(0X56,reg56val);	//对比度调节 
}
//特效设置
//0:普通模式    
//1,负片
//2,黑白   
//3,偏红色
//4,偏绿色
//5,偏蓝色
//6,复古	    
void OV7670_Special_Effects(u8 eft)
{
	u8 reg3aval=0X04;//默认为普通模式
	u8 reg67val=0XC0;
	u8 reg68val=0X80;
	switch(eft)
	{
		case 1://负片
			reg3aval=0X24;
			reg67val=0X80;
			reg68val=0X80;
			break;	
		case 2://黑白
			reg3aval=0X14;
			reg67val=0X80;
			reg68val=0X80;
			break;	
		case 3://偏红色
			reg3aval=0X14;
			reg67val=0Xc0;
			reg68val=0X80;
			break;	
		case 4://偏绿色
			reg3aval=0X14;
			reg67val=0X40;
			reg68val=0X40;
			break;	
		case 5://偏蓝色
			reg3aval=0X14;
			reg67val=0X80;
			reg68val=0XC0;
			break;	
		case 6://复古
			reg3aval=0X14;
			reg67val=0XA0;
			reg68val=0X40;
			break;	 
	}
	SCCB_WR_Reg(0X3A,reg3aval);//TSLB设置 
	SCCB_WR_Reg(0X68,reg67val);//MANU,手动U值 
	SCCB_WR_Reg(0X67,reg68val);//MANV,手动V值 
}	
//设置图像输出窗口
//对QVGA设置。
void OV7670_Window_Set(u16 sx,u16 sy,u16 width,u16 height)
{
	u16 endx;
	u16 endy;
	u8 temp; 
	endx=sx+width*2;	//V*2
 	endy=sy+height*2;
	if(endy>784)endy-=784;
	temp=SCCB_RD_Reg(0X03);				//读取Vref之前的值
	temp&=0XF0;
	temp|=((endx&0X03)<<2)|(sx&0X03);
	SCCB_WR_Reg(0X03,temp);				//设置Vref的start和end的最低2位
	SCCB_WR_Reg(0X19,sx>>2);			//设置Vref的start高8位
	SCCB_WR_Reg(0X1A,endx>>2);			//设置Vref的end的高8位

	temp=SCCB_RD_Reg(0X32);				//读取Href之前的值
	temp&=0XC0;
	temp|=((endy&0X07)<<3)|(sy&0X07);
	SCCB_WR_Reg(0X17,sy>>3);			//设置Href的start高8位
	SCCB_WR_Reg(0X18,endy>>3);			//设置Href的end的高8位
}

extern u8 ov_sta;	//在exit.c里面定义
//更新LCD显示
void camera_refresh(void)
{
	u32 j;
 	u16 color;	 
	if(ov_sta==2)
	{
		LCD_Scan_Dir(U2D_L2R);		//从上到下,从左到右 
		LCD_SetCursor(0x00,0x0000);	//设置光标位置 
		LCD_WriteRAM_Prepare();     //开始写入GRAM	
		OV7670_RRST=0;				//开始复位读指针 
		OV7670_RCK=0;
		OV7670_RCK=1;
		OV7670_RCK=0;
		OV7670_RRST=1;				//复位读指针结束 
		OV7670_RCK=1;  
		
		for(j=0;j<76800;j++)
		{
					OV7670_RCK=0;
					color = GPIOC->IDR&0XFF;	//读数据
					OV7670_RCK=1;
					
					color<<=8;
					OV7670_RCK=0;
					color |= GPIOC->IDR&0XFF;	//读数据
					OV7670_RCK=1;
					
					LCD->LCD_RAM=color;   			

		}   							 
		EXTI->PR=1<<8;     			//清除LINE8上的中断标志位
		ov_sta=0;					//开始下一次采集
		LCD_Scan_Dir(DFT_SCAN_DIR);	//恢复默认扫描方向 
	} 
}

u8 picture_send(void)
{
	u16 tx,ty,picnum=0; 	
	char *pst;							//图传数据包
	char temp[20];
	pst=(char*)mymalloc(0,1024);		//开辟至少bi4width大小的字节的内存区域 ,对240宽的屏,480个字节就够了.
	if(pst==NULL)return 1;					//内存申请失败.
	
	//TIM4->CR1&=0x00; //关闭定时器 4 ，因为摄像头 坏了，不需要使用定时器4来获取摄像头的数据了，就是不用管定时器4了
	printf ( "pciture data:" );
	/* 传输速度太慢太慢
	for(ty=0;ty<360;ty+=1)
	{
		for(tx=0;tx<240;tx+=1)
		{
			sprintf(temp,"%x",LCD_ReadPoint(tx,ty));
			strcat(pst,temp);
			picnum++;
			if(picnum==240) 
			{
				picnum=0;
				ESP8266_SendString ( DISABLE,pst,strlen( pst), Multiple_ID_0 );
				ESP8266_SendString ( DISABLE," ",strlen(" "), Multiple_ID_0 );
				pst[0]='\0';
			}
		}
	}
	*/
	for(ty=0;ty<360;ty+=3)
	{
		for(tx=0;tx<240;tx+=3)
		{
			sprintf(temp,"%x",LCD_ReadPoint(tx,ty));
			strcat(pst,temp);
			picnum++;
			if(picnum==240) 
			{
				picnum=0;
				ESP8266_SendString ( DISABLE,pst,strlen( pst), Multiple_ID_0 );
				ESP8266_SendString ( DISABLE," ",strlen(" "), Multiple_ID_0 );
				pst[0]='\0';
			}
		}
	}
	printf ( "send ok" );
	myfree(0,pst);	 	//释放内存
	//TIM4->CR1|=0x01; //使能定时器 4
	return 0;
}



















