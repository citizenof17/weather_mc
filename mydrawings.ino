#include "mydrawings.h"

void draw_cloud(U8G2_SSD1306_128X64_NONAME_F_SW_I2C &drawer) {
  drawer.drawFilledEllipse(50, 40, 8, 8);
  drawer.drawFilledEllipse(64, 38, 7, 7);
  drawer.drawFilledEllipse(74, 40, 6, 6);
  drawer.drawFilledEllipse(64, 50, 20, 10);
}


int drawer_prepare(U8G2_SSD1306_128X64_NONAME_F_SW_I2C &drawer){
    int LINE_HEIGHT = drawer.getMaxCharHeight();
    if (drawer.ty == 0){
        drawer.setCursor(0, LINE_HEIGHT);
    };
    // clear_line(drawer);
    drawer.setCursor(0, drawer.ty);
    return LINE_HEIGHT;
}

String prepare_line(auto input){
    return String(input) + "            ";
}

void print_line(U8G2_SSD1306_128X64_NONAME_F_SW_I2C &drawer, String text){
    int LINE_HEIGHT = drawer_prepare(drawer);
    drawer.print(prepare_line(text));
    drawer.setCursor(0, drawer.ty + LINE_HEIGHT);
}

void print_line(U8G2_SSD1306_128X64_NONAME_F_SW_I2C &drawer, int i){
    int LINE_HEIGHT = drawer_prepare(drawer); 
    drawer.print(prepare_line(i));
    drawer.setCursor(0, drawer.ty + LINE_HEIGHT);
}

void print_line(U8G2_SSD1306_128X64_NONAME_F_SW_I2C &drawer, float i){
    int LINE_HEIGHT = drawer_prepare(drawer); 
    drawer.print(prepare_line(i));
    drawer.setCursor(0, drawer.ty + LINE_HEIGHT);
}

void clear_line(U8G2_SSD1306_128X64_NONAME_F_SW_I2C &drawer){
    drawer.print("                ");
}
