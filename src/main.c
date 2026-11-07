/* ============================================================
 * 基于 AT89C52 的室内环境监测报警系统
 * 主控：AT89C52 / STC89C52（12MHz）
 * 采集：DS18B20（温度，单总线）、MQ-2 + ADC0832（烟雾/可燃气体）
 * 显示：4 位共阴数码管（段选 P0，位选 P2.4~P2.7）
 * 报警：蜂鸣器 P3.6、温度红灯 P1.6、烟雾黄灯 P1.7
 * 存储：片内 EEPROM（0x2000 起）保存报警阈值
 *
 * 说明：本文件为课程设计实现，由课内例程修改而来，
 *      用于学习记录与交流，非商业项目。
 * ============================================================ */

#include <reg51.h>                 //调用单片机头文件
#define uchar unsigned char        //无符号字符型宏定义
#define uint  unsigned int         //无符号整型宏定义
#include <intrins.h>
#include "eeprom52.h"
//数码管段选定义
uchar code smg_du[]={0x5f,0x44,0x9d,0xd5,0xc6,0xd3,0xdb,0x45,0xdf,0xd7,0x40,0xfd/*-*/};
//数码管位选定义
uchar code smg_we[]={0x7f,0xbf,0xdf,0xef};
uchar dis_smg[8]={0xa0,0x84,0x62,0x2a,0x39,0x2c,0x24,0xba};
sbit CS=P2^3;    //连接ADC0832CS脚
sbit SCL=P2^0;   //连接ADC0832SCL脚
sbit DO=P2^1;    //连接ADC0832DO脚
sbit ledy=P1^6;  //温度报警指示灯
sbit ledr=P1^7;  //烟雾报警指示灯
sbit dq=P1^5;    //18B20 IO口定义
sbit beep=P3^6;  //蜂鸣器IO口定义
uint temperature,s_temp;   //温度变量
uchar dengji,s_dengji;     //烟雾等级
uchar shoudong;            //手动报警键
bit flag_300ms=1;
uchar key_can;             //按键值变量
uchar menu_1;              //菜单设计变量
/* 1ms延时函数 */
void delay_1ms(uint q)
{
    uint i,j;
    for(i=0;i<q;i++)
        for(j=0;j<120;j++);
}
/* 把数据保存到单片机内部eeprom中 */
void write_eeprom()
{
    SectorErase(0x2000);
    byte_write(0x2000,s_temp);
    byte_write(0x2001,s_dengji);
    byte_write(0x2060,a_a);
}
/* 从单片机内部eeprom中读出数据 */
void read_eeprom()
{
    s_temp=byte_read(0x2000);
    s_dengji=byte_read(0x2001);
    a_a=byte_read(0x2060);
}
/* 开机自检eeprom初始化 */
void init_eeprom()
{
    read_eeprom();
    if(a_a!=1)
    {
        s_temp=50;
        s_dengji=5;
        a_a=1;
        write_eeprom();
    }
}
/* 18B20初始化函数 */
void init_18b20()
{
    bit q;
    dq=1; delay_uint(1);
    dq=0; delay_uint(80);
    dq=1; delay_uint(10);
    q=dq; delay_uint(20);
    dq=1;
}
/* 读取温度值 */
uint read_temp()
{
    uint value;
    uchar low;
    init_18b20();
    EA=0;
    write_18b20(0xcc);
    write_18b20(0x44);
    EA=1;
    delay_uint(50);
    init_18b20();
    EA=0;
    write_18b20(0xcc);
    write_18b20(0xbe);
    EA=1;
    low=read_18b20();
    value=read_18b20();
    value<<=8;
    value|=low;
    value*=0.0625;
    return value;
}
/* 主函数 */
void main()
{
    beep=0; delay_1ms(200);
    P0=P1=P2=P3=0xff;
    temperature=read_temp();
    init_eeprom();
    delay_1ms(650);
    temperature=read_temp();
    time_init();
    while(1)
    {
        key();
        if(key_can<10) key_with();
        if(flag_300ms==1)
        {
            flag_300ms=0;
            clock_h_l();
            temperature=read_temp();
            dengji=ad0832read(1,0);
            dengji=dengji*10/250;
            if(menu_1==0)
            {
                if(temperature>=99) temperature=99;
                dis_smg[3]=smg_du[dengji];
                dis_smg[2]=0x80;
                dis_smg[1]=smg_du[temperature/10%10];
                dis_smg[0]=smg_du[temperature%10];
            }
        }
        delay_1ms(1);
    }
}
