/****************************************************************************************************
//=========================================电源接线================================================//
// OLED模块               STM32单片机
//   VCC         接       DC 5V/3.3V      //OLED屏电源正
//   GND         接          GND          //OLED屏电源地
//=======================================液晶屏数据线接线==========================================//
//本模块默认数据总线类型为4线制SPI
// OLED模块               STM32单片机
//   D1          接          PB15        //OLED屏SPI写信号
//=======================================液晶屏控制线接线==========================================//
// OLED模块               STM32单片机
//   CS          接          PB11        //OLED屏片选控制信号
//   RES         接          PB12        //OLED屏复位控制信号
//   DC          接          PB10        //OLED屏数据/命令选择控制信号
//   D0          接          PB13        //OLED屏SPI时钟信号
****************************************************************************************************/	


#ifndef __OLED_H
#define __OLED_H		

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "main.h"
#include "stdlib.h"
#include "stm32f1xx_hal_gpio.h"
#include "string.h" 	 
#include "spi.h"
#include "gpio.h"
#include "CommandDef.h"

//--------------OLED参数定义---------------------
#define PAGE_SIZE			8
#define XLevelL				0x00
#define XLevelH				0x10
#define YLevel				0xB0
#define	Brightness		    0xFF 
#define WIDTH				128
#define HEIGHT				64	


//-------------写命令和数据定义-------------------
#define OLED_CMD			0		//写命令
#define OLED_DATA			1		//写数据 


//-----------------OLED端口定义----------------

#define OLED_RST_PIN		GPIO_PIN_6							//复位信号
#define OLED_RST_PORT		GPIOE								//复位信号

#define OLED_DC_PIN			GPIO_PIN_3							//数据/命令控制信号
#define OLED_DC_PORT		GPIOE								//数据/命令控制信号

#define Delay               osDelay

#define OLED_CS_PIN			GPIO_PIN_12							//片选信号
#define OLED_CS_PORT		GPIOB								//片选信号


//-----------------OLED端口操作定义----------------
 					  
#define OLED_RST_PIN_Clr()	    HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_RESET)
#define OLED_RST_PIN_Set()	    HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_SET)

#define OLED_DC_PIN_Clr()		HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_RESET)
#define OLED_DC_PIN_Set()		HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_SET)

#define OLED_CS_PIN_Clr()		HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_RESET)
#define OLED_CS_PIN_Set()		HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET)

void OledShowCtrl_HandleCommand(Command_t *cmd, Response_t *resp);
void OLED_Write_Byte(unsigned dat,unsigned cmd);														//将一个字节的内容写入OLED屏幕
void OLED_Display_On(void);																									//打开OLED显示
void OLED_Display_Off(void);																								//关闭OLED显示
void OLED_Set_Pos(unsigned char x, unsigned char y);												//在OLED屏幕中设置坐标
void OLED_Reset(void);																											//重置OLED屏幕显示
void OLED_Set_Pixel(unsigned char x, unsigned char y,unsigned char color);	//将像素值设置为RAM
void OLED_Display(void);																										//在OLED屏幕中显示
void OLED_Clear(unsigned dat);																							//清除OLED屏幕显示
void OLED_Init(void);																						//初始化OLED SSD1306控制IC

void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t color);																	//在OLED屏幕中绘制点
void OLED_Fill(uint8_t sx,uint8_t sy,uint8_t ex,uint8_t ey,uint8_t color);														//填充指定区域
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,uint8_t color);										//在两点之间画一条线
void OLED_DrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,uint8_t color);								//绘制矩形
void OLED_FillRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,uint8_t color);								//填充矩形
void OLED_DrawCircle(uint8_t xc, uint8_t yc, uint8_t color, uint8_t r);													//在指定位置绘制指定大小的圆
void OLED_FillCircle(uint8_t xc, uint8_t yc, uint8_t color, uint8_t r);													//在指定位置填充指定大小的圆
void OLED_DrawTriangel(uint8_t x0,uint8_t y0,uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2,uint8_t color);				//在指定位置绘制三角形
void OLED_FillTriangel(uint8_t x0,uint8_t y0,uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2,uint8_t color);				//在指定位置填充三角形
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t Char_Size,uint8_t mode);									//显示单个英文字符
void OLED_ShowNum(uint8_t x,uint8_t y,uint32_t num,uint8_t len,uint8_t Size,uint8_t mode);								//显示英文字符串
void OLED_ShowString(uint8_t x,uint8_t y,uint8_t *chr,uint8_t Char_Size,uint8_t mode);								//显示数字
void OLED_ShowFont16(uint8_t x,uint8_t y,uint8_t *s,uint8_t mode);															//显示单个16x16汉字
void OLED_ShowFont24(uint8_t x,uint8_t y,uint8_t *s,uint8_t mode);															//显示单个24x24汉字
void OLED_ShowFont32(uint8_t x,uint8_t y,uint8_t *s,uint8_t mode);															//显示单个32x32汉字
void OLED_ShowCHinese(uint8_t x,uint8_t y,uint8_t hsize,uint8_t *str,uint8_t mode);									//显示中文字符串
void OLED_DrawBMP(uint8_t x,uint8_t y,uint8_t width, uint8_t height, uint8_t BMP[], uint8_t mode);				//显示BMP单色图片

void OLED_Test_MainPage(void);																							//绘制综合测试程序主界面
void OLED_Test_Color(void);																									//颜色填充测试（白色、黑色）
void OLED_Test_Rectangular(void);																						//矩形显示和填充测试
void OLED_Test_Circle(void);																								//循环显示和填充测试依次显示黑白圆形框，1000毫秒后，依次用黑色、白色填充圆形
void OLED_Test_Triangle(void);																							//三角形显示和填充测试依次显示黑白三角形框，1000毫秒后，依次用黑色和白色填充三角形
void OLED_TEST_English(void);																								//英文显示测试
void OLED_TEST_Number_Character(void);																			//数字和字符显示测试
void OLED_Test_Chinese(void);																								//中文显示测试
void OLED_Test_BMP(void);																										//BMP单色图片显示测试
void OLED_Test_Menu1(void);																									//中文选择菜单显示测试
void OLED_Test_Menu2(void);																									//英文天气界面显示测试
void OLED_Test_Menu3(void);

#endif


