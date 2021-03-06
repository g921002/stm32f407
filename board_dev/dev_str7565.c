/**
 * @file            dev_cog12864.c
 * @brief           COG LCD驱动
 * @author          wujique
 * @date            2018年1月10日 星期三
 * @version         初稿
 * @par             版权所有 (C), 2013-2023
 * @par History:
 * 1.日    期:        2018年1月10日 星期三
 *   作    者:         屋脊雀工作室
 *   修改内容:   创建文件
		版权说明：
		1 源码归屋脊雀工作室所有。
		2 可以用于的其他商业用途（配套开发板销售除外），不须授权。
		3 屋脊雀工作室不对代码功能做任何保证，请使用者自行测试，后果自负。
		4 可随意修改源码并分发，但不可直接销售本代码获利，并且保留版权说明。
		5 如发现BUG或有优化，欢迎发布更新。请联系：code@wujique.com
		6 使用本源码则相当于认同本版权说明。
		7 如侵犯你的权利，请联系：code@wujique.com
		8 一切解释权归屋脊雀工作室所有。
*/
#include <stdarg.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "main.h"
#include "wujique_log.h"
#include "alloc.h"
#include "dev_lcdbus.h"
#include "dev_lcd.h"
#include "dev_str7565.h"

/*

	COG LCD 的驱动

*/
/*-----------------------------


------------------------------*/
/*
	驱动使用的数据结构，不对外
*/
struct _cog_drv_data
{
	u8 gram[8][128];	
};	



#define TFT_LCD_DRIVER_COG12864

#ifdef TFT_LCD_DRIVER_COG12864

s32 drv_ST7565_init(DevLcd *lcd);
static s32 drv_ST7565_drawpoint(DevLcd *lcd, u16 x, u16 y, u16 color);
s32 drv_ST7565_color_fill(DevLcd *lcd, u16 sx,u16 ex,u16 sy,u16 ey,u16 color);
s32 drv_ST7565_fill(DevLcd *lcd, u16 sx,u16 ex,u16 sy,u16 ey,u16 *color);
static s32 drv_ST7565_display_onoff(DevLcd *lcd, u8 sta);
s32 drv_ST7565_prepare_display(DevLcd *lcd, u16 sx, u16 ex, u16 sy, u16 ey);
static void drv_ST7565_scan_dir(DevLcd *lcd, u8 dir);
void drv_ST7565_lcd_bl(DevLcd *lcd, u8 sta);

/*

	定义一个TFT LCD，使用ST7565驱动IC的设备

*/
_lcd_drv CogLcdST7565Drv = {
							.id = 0X7565,

							.init = drv_ST7565_init,
							.draw_point = drv_ST7565_drawpoint,
							.color_fill = drv_ST7565_color_fill,
							.fill = drv_ST7565_fill,
							.onoff = drv_ST7565_display_onoff,
							.prepare_display = drv_ST7565_prepare_display,
							.set_dir = drv_ST7565_scan_dir,
							.backlight = drv_ST7565_lcd_bl
							};
/*

	这是一个衍生出来的ST7565驱动，
	ID叫做7564，用处是：
	如果一个系统有两个ST7565的LCD，
	一个是128*64，一个是128*32。
	如果设备ID跟驱动ID通过映射表对应，就没有问题，
	我们暂时不做映射，直接设备ID就是驱动ID。
	因此需要区分两种不同设备，只好衍生一种假的驱动，
	这种驱动除了ID不一样，其他跟ST7565完全一样。

*/
_lcd_drv CogLcdST7564Drv = {
							.id = 0X7564,

							.init = drv_ST7565_init,
							.draw_point = drv_ST7565_drawpoint,
							.color_fill = drv_ST7565_color_fill,
							.fill = drv_ST7565_fill,
							.onoff = drv_ST7565_display_onoff,
							.prepare_display = drv_ST7565_prepare_display,
							.set_dir = drv_ST7565_scan_dir,
							.backlight = drv_ST7565_lcd_bl
							};

void drv_ST7565_lcd_bl(DevLcd *lcd, u8 sta)
{
	_lcd_bus *LcdBusDrv;
	LcdBusDrv = dev_lcdbus_find(lcd->dev->bus);
	LcdBusDrv->bl(sta);
}
	
/**
 *@brief:      drv_ST7565_scan_dir
 *@details:    设置显存扫描方向， 本函数为竖屏角度
 *@param[in]   u8 dir  
 *@param[out]  无
 *@retval:     static
 */
static void drv_ST7565_scan_dir(DevLcd *lcd, u8 dir)
{
	return;
}

/**
 *@brief:      drv_ST7565_set_cp_addr
 *@details:    设置控制器的行列地址范围
 *@param[in]   u16 sc  
               u16 ec  
               u16 sp  
               u16 ep  
 *@param[out]  无
 *@retval:     
 */
#if 0
static s32 drv_ST7565_set_cp_addr(DevLcd *lcd, u16 sc, u16 ec, u16 sp, u16 ep)
{
	return 0;
}
#endif
/**
 *@brief:      drv_ST7565_refresh_gram
 *@details:       刷新指定区域到屏幕上
                  坐标是横屏模式坐标
 *@param[in]   u16 sc  
               u16 ec  
               u16 sp  
               u16 ep  
 *@param[out]  无
 *@retval:     static
 */
static s32 drv_ST7565_refresh_gram(DevLcd *lcd, u16 sc, u16 ec, u16 sp, u16 ep)
{	
	struct _cog_drv_data *gram; 
	u8 i;
	
	_lcd_bus *LcdBusDrv;
	LcdBusDrv = dev_lcdbus_find(lcd->dev->bus);
	//uart_printf("drv_ST7565_refresh:%d,%d,%d,%d\r\n", sc,ec,sp,ep);
	gram = (struct _cog_drv_data *)lcd->pri;
	
	LcdBusDrv->open();
    for(i=sp/8; i <= ep/8; i++)
    {
        LcdBusDrv->writecmd (0xb0+i);    //设置页地址（0~7）
        LcdBusDrv->writecmd (((sc>>4)&0x0f)+0x10);      //设置显示位置—列高地址
        LcdBusDrv->writecmd (sc&0x0f);      //设置显示位置—列低地址

         LcdBusDrv->writedata(&(gram->gram[i][sc]), ec-sc+1);

	}
	LcdBusDrv->close();
	
	return 0;
}

/**
 *@brief:      drv_ST7565_display_onoff
 *@details:    显示或关闭
 *@param[in]   u8 sta  
 *@param[out]  无
 *@retval:     static
 */
static s32 drv_ST7565_display_onoff(DevLcd *lcd, u8 sta)
{
	_lcd_bus *LcdBusDrv;
	LcdBusDrv = dev_lcdbus_find(lcd->dev->bus);
	
	LcdBusDrv->open();
	if(sta == 1)
	{
		LcdBusDrv->writecmd(0XCF);  //DISPLAY ON
	}
	else
	{
		LcdBusDrv->writecmd(0XCE);  //DISPLAY OFF	
	}
	LcdBusDrv->close();
	return 0;
}

/**
 *@brief:      drv_ST7565_init
 *@details:    
 *@param[in]   void  
 *@param[out]  无
 *@retval:     
 */
s32 drv_ST7565_init(DevLcd *lcd)
{
	_lcd_bus *LcdBusDrv;
	LcdBusDrv = dev_lcdbus_find(lcd->dev->bus);
	
	LcdBusDrv->init();
	LcdBusDrv->open();
	
	LcdBusDrv->writecmd(0xe2);//软复位
	Delay(50);
	LcdBusDrv->writecmd(0x2c);//升压步骤1
	Delay(50);
	LcdBusDrv->writecmd(0x2e);//升压步骤2
	Delay(50);
	LcdBusDrv->writecmd(0x2f);//升压步骤3
	Delay(50);
	
	LcdBusDrv->writecmd(0x24);//对比度粗调，范围0X20，0X27
	LcdBusDrv->writecmd(0x81);//对比度微调
	LcdBusDrv->writecmd(0x25);//对比度微调值 0x00-0x3f
	
	LcdBusDrv->writecmd(0xa2);// 偏压比
	LcdBusDrv->writecmd(0xc8);//行扫描，从上到下
	LcdBusDrv->writecmd(0xa0);//列扫描，从左到右
	LcdBusDrv->writecmd(0x40);//起始行，第一行
	LcdBusDrv->writecmd(0xaf);//开显示

	LcdBusDrv->close();
	
	wjq_log(LOG_INFO, "drv_ST7565_init finish\r\n");

	/*申请显存，永不释放*/
	lcd->pri = (void *)wjq_malloc(sizeof(struct _cog_drv_data));
	memset((char*)lcd->pri, 0x00, 128*8);//要改为动态判断显存大小
	
	drv_ST7565_refresh_gram(lcd, 0,127,0,63);

	return 0;
}

/**
 *@brief:      drv_ST7565_xy2cp
 *@details:    将xy坐标转换为CP坐标
 			   对于COG黑白屏来说，CP坐标就是横屏坐标，
 			   转竖屏后，CP坐标还是横屏坐标，
 			   也就是说当竖屏模式，就需要将XY坐标转CP坐标
 			   横屏不需要转换
 *@param[in]   无
 *@param[out]  无
 *@retval:     
 */
s32 drv_ST7565_xy2cp(DevLcd *lcd, u16 sx, u16 ex, u16 sy, u16 ey, u16 *sc, u16 *ec, u16 *sp, u16 *ep)
{

	return 0;
}
/**
 *@brief:      drv_ST7565_drawpoint
 *@details:    画点
 *@param[in]   u16 x      
               u16 y      
               u16 color  
 *@param[out]  无
 *@retval:     static
 */
static s32 drv_ST7565_drawpoint(DevLcd *lcd, u16 x, u16 y, u16 color)
{
	u16 xtmp,ytmp;
	u16 page, colum;

	struct _cog_drv_data *gram;
	_lcd_bus *LcdBusDrv;
	LcdBusDrv = dev_lcdbus_find(lcd->dev->bus);
	
	gram = (struct _cog_drv_data *)lcd->pri;

	if(x > lcd->width)
		return -1;
	if(y > lcd->height)
		return -1;

	if(lcd->dir == W_LCD)
	{
		xtmp = x;
		ytmp = y;
	}
	else//如果是竖屏，XY轴跟显存的映射要对调
	{
		xtmp = y;
		ytmp = x;
	}
	
	page = ytmp/8; //页地址
	colum = xtmp;//列地址
	
	if(color == BLACK)
	{
		gram->gram[page][colum] |= (0x01<<(ytmp%8));
	}
	else
	{
		gram->gram[page][colum] &= ~(0x01<<(ytmp%8));
	}

	/*效率不高*/
	LcdBusDrv->open();
    LcdBusDrv->writecmd (0xb0 + page );   
    LcdBusDrv->writecmd (((colum>>4)&0x0f)+0x10); 
    LcdBusDrv->writecmd (colum&0x0f);    
    LcdBusDrv->writedata( &(gram->gram[page][colum]), 1);
	LcdBusDrv->close();
	return 0;
}
/**
 *@brief:      drv_ST7565_color_fill
 *@details:    将一块区域设定为某种颜色
 *@param[in]   u16 sx     
               u16 sy     
               u16 ex     
               u16 ey     
               u16 color  
 *@param[out]  无
 *@retval:     
 */
s32 drv_ST7565_color_fill(DevLcd *lcd, u16 sx,u16 ex,u16 sy,u16 ey,u16 color)
{
	u16 i,j;
	u16 xtmp,ytmp;
	u16 page, colum;

	
	struct _cog_drv_data *gram;

	//uart_printf("drv_ST7565_fill:%d,%d,%d,%d\r\n", sx,ex,sy,ey);

	gram = (struct _cog_drv_data *)lcd->pri;

	/*防止坐标溢出*/
	if(sy >= lcd->height)
	{
		sy = lcd->height-1;
	}
	if(sx >= lcd->width)
	{
		sx = lcd->width-1;
	}
	
	if(ey >= lcd->height)
	{
		ey = lcd->height-1;
	}
	if(ex >= lcd->width)
	{
		ex = lcd->width-1;
	}
	
	for(j=sy;j<=ey;j++)
	{
		//uart_printf("\r\n");
		
		for(i=sx;i<=ex;i++)
		{

			if(lcd->dir == W_LCD)
			{
				xtmp = i;
				ytmp = j;
			}
			else//如果是竖屏，XY轴跟显存的映射要对调
			{
				xtmp = j;
				ytmp = lcd->width-i;
			}

			page = ytmp/8; //页地址
			colum = xtmp;//列地址
			
			if(color == BLACK)
			{
				gram->gram[page][colum] |= (0x01<<(ytmp%8));
				//uart_printf("*");
			}
			else
			{
				gram->gram[page][colum] &= ~(0x01<<(ytmp%8));
				//uart_printf("-");
			}
		}
	}

	/*
		只刷新需要刷新的区域
		坐标范围是横屏模式
	*/
	if(lcd->dir == W_LCD)
	{
		drv_ST7565_refresh_gram(lcd, sx,ex,sy,ey);
	}
	else
	{
		drv_ST7565_refresh_gram(lcd, sy, ey, lcd->width-ex-1, lcd->width-sx-1); 	
	}
		
	return 0;
}


/**
 *@brief:      drv_ST7565_color_fill
 *@details:    填充矩形区域
 *@param[in]   u16 sx      
               u16 sy      
               u16 ex      
               u16 ey      
               u16 *color  每一个点的颜色数据
 *@param[out]  无
 *@retval:     
 */
s32 drv_ST7565_fill(DevLcd *lcd, u16 sx,u16 ex,u16 sy,u16 ey,u16 *color)
{
	u16 i,j;
	u16 xtmp,ytmp;
	u16 xlen,ylen;
	u16 page, colum;
	u32 index;
	
	struct _cog_drv_data *gram;

	//uart_printf("drv_ST7565_fill:%d,%d,%d,%d\r\n", sx,ex,sy,ey);

	gram = (struct _cog_drv_data *)lcd->pri;

	/*xlen跟ylen是用来取数据的，不是填LCD*/
	xlen = ex-sx+1;//全包含
	ylen = ey-sy+1;

	/*防止坐标溢出*/
	if(sy >= lcd->height)
	{
		sy = lcd->height-1;
	}
	if(sx >= lcd->width)
	{
		sx = lcd->width-1;
	}
	
	if(ey >= lcd->height)
	{
		ey = lcd->height-1;
	}
	if(ex >= lcd->width)
	{
		ex = lcd->width-1;
	}
	
	for(j=sy;j<=ey;j++)
	{
		//uart_printf("\r\n");
		index = (j-sy)*xlen;
		
		for(i=sx;i<=ex;i++)
		{

			if(lcd->dir == W_LCD)
			{
				xtmp = i;
				ytmp = j;
			}
			else//如果是竖屏，XY轴跟显存的映射要对调
			{
				xtmp = j;
				ytmp = lcd->width-i;
			}

			page = ytmp/8; //页地址
			colum = xtmp;//列地址
			
			if(*(color+index+i-sx) == BLACK)
			{
				gram->gram[page][colum] |= (0x01<<(ytmp%8));
				//uart_printf("*");
			}
			else
			{
				gram->gram[page][colum] &= ~(0x01<<(ytmp%8));
				//uart_printf("-");
			}
		}
	}

	/*
		只刷新需要刷新的区域
		坐标范围是横屏模式
	*/
	if(lcd->dir == W_LCD)
	{
		drv_ST7565_refresh_gram(lcd, sx,ex,sy,ey);
	}
	else
	{

		drv_ST7565_refresh_gram(lcd, sy, ey, lcd->width-ex-1, lcd->width-sx-1); 	
	}
	//uart_printf("refresh ok\r\n");		
	return 0;
}

s32 drv_ST7565_prepare_display(DevLcd *lcd, u16 sx, u16 ex, u16 sy, u16 ey)
{
	return 0;
}
#endif


/*

	OLED 跟 COG LCD 操作类似
	仅仅初始化不一样
	OLED有两种驱动，SSD1315，SSD1615，一样的。
*/

/**
 *@brief:	   drv_ssd1615_init
 *@details:    
 *@param[in]   void  
 *@param[out]  无
 *@retval:	   
 */
s32 drv_ssd1615_init(DevLcd *lcd)
{
	_lcd_bus *LcdBusDrv;
	LcdBusDrv = dev_lcdbus_find(lcd->dev->bus);
	
	LcdBusDrv->init();

	LcdBusDrv->open();

	LcdBusDrv->writecmd(0xAE);//--turn off oled panel
	LcdBusDrv->writecmd(0x00);//---set low column address
	LcdBusDrv->writecmd(0x10);//---set high column address
	LcdBusDrv->writecmd(0x40);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	LcdBusDrv->writecmd(0x81);//--set contrast control register
	LcdBusDrv->writecmd(0xCF); // Set SEG Output Current Brightness
	LcdBusDrv->writecmd(0xA1);//--Set SEG/Column Mapping	  0xa0左右反置 0xa1正常
	LcdBusDrv->writecmd(0xC8);//Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
	LcdBusDrv->writecmd(0xA6);//--set normal display
	LcdBusDrv->writecmd(0xA8);//--set multiplex ratio(1 to 64)
	LcdBusDrv->writecmd(0x3f);//--1/64 duty
	LcdBusDrv->writecmd(0xD3);//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	LcdBusDrv->writecmd(0x00);//-not offset
	LcdBusDrv->writecmd(0xd5);//--set display clock divide ratio/oscillator frequency
	LcdBusDrv->writecmd(0x80);//--set divide ratio, Set Clock as 100 Frames/Sec
	LcdBusDrv->writecmd(0xD9);//--set pre-charge period
	LcdBusDrv->writecmd(0xF1);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	LcdBusDrv->writecmd(0xDA);//--set com pins hardware configuration
	LcdBusDrv->writecmd(0x12);
	LcdBusDrv->writecmd(0xDB);//--set vcomh
	LcdBusDrv->writecmd(0x40);//Set VCOM Deselect Level
	LcdBusDrv->writecmd(0x20);//-Set Page Addressing Mode (0x00/0x01/0x02)
	LcdBusDrv->writecmd(0x02);//
	LcdBusDrv->writecmd(0x8D);//--set Charge Pump enable/disable
	LcdBusDrv->writecmd(0x14);//--set(0x10) disable
	LcdBusDrv->writecmd(0xA4);// Disable Entire Display On (0xa4/0xa5)
	LcdBusDrv->writecmd(0xA6);// Disable Inverse Display On (0xa6/a7) 
	LcdBusDrv->writecmd(0xAF);//--turn on oled panel

	LcdBusDrv->writecmd(0xAF);//--turn on oled panel 
	LcdBusDrv->close();
	wjq_log(LOG_INFO, "dev_ssd1615_init finish\r\n");

	lcd->pri = (void *)wjq_malloc(sizeof(struct _cog_drv_data));
	memset((char*)lcd->pri, 0x00, 128*8);//要改为动态判断显存大小
	
	drv_ST7565_refresh_gram(lcd, 0,127,0,63);

	return 0;
}

/**
 *@brief:      drv_ssd1615_display_onoff
 *@details:    SSD1615打开或关闭显示
 *@param[in]   DevLcd *lcd  
               u8 sta       
 *@param[out]  无
 *@retval:     
 */
s32 drv_ssd1615_display_onoff(DevLcd *lcd, u8 sta)
{
	_lcd_bus *LcdBusDrv;
	LcdBusDrv = dev_lcdbus_find(lcd->dev->bus);
	
	LcdBusDrv->open();
	if(sta == 1)
	{
    	LcdBusDrv->writecmd(0X8D);  //SET DCDC命令
    	LcdBusDrv->writecmd(0X14);  //DCDC ON
    	LcdBusDrv->writecmd(0XAF);  //DISPLAY ON
	}
	else
	{
		LcdBusDrv->writecmd(0X8D);  //SET DCDC命令
    	LcdBusDrv->writecmd(0X10);  //DCDC OFF
    	LcdBusDrv->writecmd(0XAE);  //DISPLAY OFF	
	}
	LcdBusDrv->close();
	
	return 0;
}

_lcd_drv OledLcdSSD1615rv = {
							.id = 0X1315,

							.init = drv_ssd1615_init,
							.draw_point = drv_ST7565_drawpoint,
							.color_fill = drv_ST7565_color_fill,
							.fill = drv_ST7565_fill,
							.onoff = drv_ssd1615_display_onoff,
							.prepare_display = drv_ST7565_prepare_display,
							.set_dir = drv_ST7565_scan_dir,
							.backlight = drv_ST7565_lcd_bl
							};


