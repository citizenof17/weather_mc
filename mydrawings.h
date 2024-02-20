#ifndef MY_DRAWINGS
#define MY_DRAWINGS

#include <U8g2lib.h>

int drawer_prepare(U8G2_SSD1306_128X64_NONAME_F_SW_I2C &drawer);
void draw_cloud(U8G2_SSD1306_128X64_NONAME_F_SW_I2C &drawer);
void print_line(U8G2_SSD1306_128X64_NONAME_F_SW_I2C &drawer, String text);
void print_line(U8G2_SSD1306_128X64_NONAME_F_SW_I2C &drawer, int i);
void print_line(U8G2_SSD1306_128X64_NONAME_F_SW_I2C &drawer, float i);
void clear_line(U8G2_SSD1306_128X64_NONAME_F_SW_I2C &drawer);

#endif