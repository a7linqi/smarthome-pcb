#ifndef __OLED_H
#define __OLED_H

#include "main.h"

/*
 * OLED 显示屏驱动（0.96寸 SSD1306，128x64 像素，I2C接口）
 * 引脚：SCL -> PB8，SDA -> PB9
 *
 * 显示区域划分：
 * - 共 4 行，每行 16 个字符
 * - 行号范围：1~4
 * - 列号范围：1~16
 */

/* 初始化 OLED 显示屏
 * 功能：配置 I2C 引脚，发送初始化命令，清屏
 * 调用时机：程序开始时调用一次
 * 示例：OLED_Init();
 */
void OLED_Init(void);

/* 清屏
 * 功能：清除整个屏幕内容（128x64 像素全部置0）
 * 示例：OLED_Clear();
 */
void OLED_Clear(void);

/* 显示一个字符
 * 参数：
 *   Line   - 行位置，范围 1~4
 *   Column - 列位置，范围 1~16
 *   Char   - 要显示的字符，范围 ASCII 可见字符（空格~波浪号）
 * 示例：OLED_ShowChar(1, 1, 'A');  // 在第1行第1列显示 'A'
 */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);

/* 显示字符串
 * 参数：
 *   Line   - 起始行位置，范围 1~4
 *   Column - 起始列位置，范围 1~16
 *   String - 要显示的字符串
 * 示例：OLED_ShowString(1, 1, "Hello");  // 在第1行第1列显示 "Hello"
 */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);

/* 显示无符号十进制数字
 * 参数：
 *   Line   - 起始行位置，范围 1~4
 *   Column - 起始列位置，范围 1~16
 *   Number - 要显示的数字，范围 0~4294967295
 *   Length - 显示位数，不足补零，范围 1~10
 * 示例：OLED_ShowNum(1, 1, 123, 5);  // 显示 "00123"
 */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/* 显示带符号十进制数字
 * 参数：
 *   Line   - 起始行位置，范围 1~4
 *   Column - 起始列位置，范围 1~16
 *   Number - 要显示的数字，范围 -2147483648~2147483647
 *   Length - 数字部分显示位数（不含正负号），范围 1~10
 * 示例：OLED_ShowSignedNum(1, 1, -123, 3);  // 显示 "-123"
 */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);

/* 显示十六进制数字
 * 参数：
 *   Line   - 起始行位置，范围 1~4
 *   Column - 起始列位置，范围 1~16
 *   Number - 要显示的数字，范围 0~0xFFFFFFFF
 *   Length - 显示位数，不足补零，范围 1~8
 * 示例：OLED_ShowHexNum(1, 1, 0xFF, 4);  // 显示 "00FF"
 */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/* 显示二进制数字
 * 参数：
 *   Line   - 起始行位置，范围 1~4
 *   Column - 起始列位置，范围 1~16
 *   Number - 要显示的数字，范围 0~65535
 *   Length - 显示位数，不足补零，范围 1~16
 * 示例：OLED_ShowBinNum(1, 1, 0x0F, 8);  // 显示 "00001111"
 */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/* 显示一个中文字符
 * 参数：
 *   Line   - 行位置，范围 1~4
 *   Column - 列位置，范围 1~16（一个中文占2列）
 *   ChineseIndex - 中文字模索引，范围 0~15
 *
 * 注意：一个中文字符占 16x16 像素，相当于 2 个 ASCII 字符宽度
 *       所以 Column 每次增加 2
 *
 * 可用中文（索引值）：
 *   0=温  1=度  2=湿  3=智  4=能  5=家  6=居
 *   7=灯  8=开  9=关  10=空 11=调 12=光 13=照
 *   14=报 15=警
 *
 * 示例：OLED_ShowChinese(1, 1, 0);   // 在第1行第1列显示 "温"
 *       OLED_ShowChinese(1, 3, 1);   // 在第1行第3列显示 "度"
 *       OLED_ShowChinese(1, 5, 2);   // 在第1行第5列显示 "湿"
 */
void OLED_ShowChinese(uint8_t Line, uint8_t Column, uint8_t ChineseIndex);

#endif /* __OLED_H */
