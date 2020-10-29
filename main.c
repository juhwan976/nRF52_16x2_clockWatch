#include <stdbool.h> //to use bool type
#include <stdio.h> //to use sprintf
#include <string.h> //to use strlen
#include <math.h> //to use pow

#include "nrf52.h"
#include "nrf52_bitfields.h"
#include "cssp_gpio.h"

/*-----------------------------------Defines-----------------------------------*/
// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define En 0b00000100  // Enable bit
#define Rw 0b00000010  // Read/Write bit
#define Rs 0b00000001  // Register select bit

/*--------------------------------we have these functions-------------------------------------*/
//void cssp_delay_125(uint8_t count);
void cssp_delay_us(uint32_t count);
void cssp_twi_suspend(void);

void expanderWrite(uint8_t _data);
void write4bits(uint8_t value);
void send(uint8_t value, uint8_t mode);
void pulseEnable(uint8_t _data);
void command(uint8_t value);
void write(uint8_t value);
void lcd_begin(void);
void cssp_lcd_print(const char* str);
void cssp_lcd_clear(void);

char* clock_findDow(uint8_t dow);
void clock_addSec(void);
void clock_addStopwatch(void);
void clock_checkAlarm(void);
bool clock_isYoon(uint16_t year);
void clock_buttonMenu(uint8_t button, void function_print(void), void function_do(void));
void clock_main(void);
void clock_timeSetting_print(uint8_t item, bool onf);
void clock_timeSetting_increse(uint8_t item);
void clock_timeSetting_decrese(uint8_t item);
void clock_timeSetting_nextItem(void);
void clock_timeSetting_do(void);
void clock_stopWatch_print(bool nclear);
void clock_stopWatch_startnStop(void);
void clock_stopWatch_do(void);
void clock_alarmSetting_print(uint8_t a_item, bool onf);
void clock_alarmSetting_increse(uint8_t a_item);
void clock_alarmSetting_decrese(uint8_t a_item);
void clock_alarmSetting_nextItem(void);
void clock_alarmSetting_do(void);

uint16_t year = 2020; //min : 1970, max : 9999
uint8_t month = 1, date = 1, dow = 1;
uint8_t hour = 1, min = 0, sec = 0;
uint8_t a_hour = 1, a_min = 0;
uint8_t s_hour = 0, s_min = 0, s_sec = 0, s_ms = 0;
uint8_t item = 1, a_item = 1;
bool alarm = true, AM = true, a_AM = true, timerWork = false;
bool is = false, alarmCheck = false;
char buf[64]; //buffer string

uint8_t tick = 0;
bool still = false; //pushing button check by TIMER3
bool isbutton = false; //did you push button?
bool stillbutton = false; //pushing button check by function
/*-------------------------------------IRQ Handler-------------------------------------------*/
void RTC0_IRQHandler() {
    if(NRF_RTC0->EVENTS_TICK--) {
        if(++tick == 8) {
            clock_addSec();
            clock_checkAlarm();
            cssp_gpio_invert(17);
            tick = 0;
            NRF_RTC0->TASKS_CLEAR = 1;
        }
    }
}

void TIMER3_IRQHandler() {
    if(NRF_TIMER3->EVENTS_COMPARE[0]--) {    
        clock_addStopwatch();
        NRF_TIMER3->TASKS_CLEAR = 1;
    }
}

void TIMER4_IRQHandler() {
    if(NRF_TIMER4->EVENTS_COMPARE[0]--) {
        still = true;
        NRF_TIMER4->TASKS_CLEAR = 1;
    }
}

/*-----------------------------------define LCD functions-----------------------------------*/
/* low level */
void expanderWrite(uint8_t _data) {
    NRF_TWI0->TXD = ((int)_data | LCD_BACKLIGHT);
    NRF_TWI0->TASKS_RESUME = 1;
    cssp_twi_suspend();
}

void write4bits(uint8_t value) {
    expanderWrite(value);
    pulseEnable(value);
}

void send(uint8_t value, uint8_t mode) {
    uint8_t highnib = value & 0xF0;
    uint8_t lownib = (value<<4) & 0xF0;
    write4bits((highnib)|mode);
    write4bits((lownib)|mode);
}

void pulseEnable(uint8_t _data) {
    expanderWrite(_data | En);
    cssp_delay_us(50);

    expanderWrite(_data & (~En));
    cssp_delay_us(50);
}

/* mid level */
void command(uint8_t value) {
    send(value, 0);
}

void write(uint8_t value) {
    send(value, Rs);
}

/* etc */
void lcd_begin(void){
    uint8_t _displayfunction = (LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS);
    cssp_delay_us(50);

    NRF_TWI0->TXD = LCD_BACKLIGHT;
    NRF_TWI0->TASKS_STARTTX = 1;
    cssp_twi_suspend();
    cssp_delay_us(1000);

    write4bits(0x03 << 4);
    cssp_delay_us(4500);

    write4bits(0x03 << 4);
    cssp_delay_us(4500);

    write4bits(0x03 << 4);
    cssp_delay_us(150);

    write4bits(0x02 << 4);

    command(LCD_FUNCTIONSET | _displayfunction);

    command(LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF);
    command(LCD_CLEARDISPLAY); //function clear();
    cssp_delay_us(2000);

    command(LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT);

    command(LCD_RETURNHOME); //function home();
    cssp_delay_us(2000);
}

void cssp_lcd_print(const char *str) {
    for(int i = 0 ; i < strlen(str) ; i++) {
        write(str[i]);
    }
}

void cssp_lcd_clear(void) {
    command(0b10000000);
    sprintf(buf, "                ");
    cssp_lcd_print(buf);

    command(0b11000000);
    cssp_lcd_print(buf);
}

/*-----------------------------------define CSSP functions-----------------------------------*/
//void cssp_delay_125(uint8_t count) {
//    NRF_RTC0->TASKS_START = 1;
//    while(1) {
//        if(NRF_RTC0->COUNTER == count)
//            break;
//    }
//    NRF_RTC0->TASKS_STOP = 1;
//    NRF_RTC0->TASKS_CLEAR = 1;
//}

void cssp_delay_us(uint32_t count) {
    NRF_TIMER0->CC[0] = 16000000 / pow(2, NRF_TIMER0->PRESCALER) * count / 1000000;
    NRF_TIMER0->TASKS_START = 1;
    while(1) {
        if(NRF_TIMER0->EVENTS_COMPARE[0]--)
            break;
    }
    NRF_TIMER0->TASKS_STOP = 1;
    NRF_TIMER0->TASKS_CLEAR = 1;
}

void cssp_twi_suspend() {
    while(1) {
        if(NRF_TWI0->EVENTS_SUSPENDED--)
            break;
    }
}

/*-------------------------------------Clock function----------------------------------------*/
char* clock_findDow(uint8_t dow) {
    switch(dow) {
        case 1:
            return "MON";
            break;
        case 2:
            return "TUE";
            break;
        case 3:
            return "WED";
            break;
        case 4:
            return "THU";
            break;
        case 5:
            return "FRI";
            break;
        case 6:
            return "SAT";
            break;
        case 7:
            return "SUN";
            break;
        default :
            return "???";
            break;
    }
}

void clock_addSec(void) {
    sec++;
    if(sec == 60) {
        min++;
        sec = 0;
    }

    if(min == 60) {
        hour++;
        min = 0;
    }

    if(hour == 13) {
        if(AM)
            AM = false;
        else {
            AM = true;
            date++;
            dow++;
        }
        hour = 1;
    }
    
    if(dow == 8)
        dow = 1;

    if(month == 2 && clock_isYoon(year) && date == 30) {
        month++;
        date = 1;
    }
    else if(month == 2 && !clock_isYoon(year) && date == 29) {
        month++;
        date = 1;
    }
    else if((month == 1||month == 3||month == 5||month == 7||month == 8||month == 10||month == 12) && date == 32) {
        month++;
        date = 1;
    }
    else if((month == 4||month == 6||month == 9||month == 11) && date == 31) {
        month++;
        date = 1;
    }

    if(month == 13) {
        year++;
        month = 1;
    }

    if(year == 10000) {
        year = 1970;
    }
    
}

void clock_addStopwatch(void) {
    s_ms++;
    if(s_ms == 100) {
        s_sec++;
        s_ms = 0;
    }

    if(s_sec == 60) {
        s_min++;
        s_sec = 0;
    }

    if(s_min == 60) {
        s_hour++;
        s_min = 0;
    }

    if(s_hour == 100) {
        s_hour = 0;
    }
}

void clock_checkAlarm(void) {
    if(alarm && (a_AM == AM) && (a_hour == hour) && (a_min == min) && (sec == 0))
        alarmCheck = true;
}

bool clock_isYoon(uint16_t year) {
    if((year%4 == 0)&&(year%100)&&(year%400))
        return true;
    else if((year%4 == 0)&&(year%100))
        return false;
    else if((year%4 == 0))
        return true;
    else 
        return false;
}

void clock_buttonMenu(uint8_t button, void function_print(void), void function_do(void)) {
    bool out1 = false, out2 = false, m_isbutton = false;

    while(1) {
        if((!cssp_gpio_read(button)) && (!m_isbutton)) {   
            function_print();
            m_isbutton = true;
            out1 = true;
        }
        else if(!cssp_gpio_read(button)) {
            // do nothing
        }
        else {
            m_isbutton = false;
            if(out1)
                out2 = true;
        }
        
        if(out1&out2)
            break;
    }

    function_do();
}

void clock_main(void) {
    command(0b10000000);
    sprintf(buf, "%04d/%02d/%02d %s %c", year, month, date, clock_findDow(dow), alarm?'A':' ');
    cssp_lcd_print(buf);

    command(0b11000000);
    sprintf(buf, "%s %02d:%02d:%02d      ", AM?"AM":"PM", hour, min, sec);
    cssp_lcd_print(buf);
}

void clock_timeSetting_print(uint8_t item, bool onf) {
    if((item == 1) && (!onf)) {
        command(0b10000000);
        sprintf(buf, "    /%02d/%02d %s  ", month, date, clock_findDow(dow));
        cssp_lcd_print(buf);
        command(0b11000000);
        sprintf(buf, "%s %02d:%02d:%02d     ", AM?"AM":"PM", hour, min, sec);
        cssp_lcd_print(buf);
    }
    else if((item == 2) && (!onf)) {
        command(0b10000000);
        sprintf(buf, "%04d/  /%02d %s  ", year, date, clock_findDow(dow));
        cssp_lcd_print(buf);
        command(0b11000000);
        sprintf(buf, "%s %02d:%02d:%02d     ", AM?"AM":"PM", hour, min, sec);
        cssp_lcd_print(buf);
    }
    else if((item == 3) && (!onf)) {
        command(0b10000000);
        sprintf(buf, "%04d/%02d/   %s  ", year, month, clock_findDow(dow));
        cssp_lcd_print(buf);
        command(0b11000000);
        sprintf(buf, "%s %02d:%02d:%02d     ", AM?"AM":"PM", hour, min, sec);
        cssp_lcd_print(buf);
    }
    else if((item == 4) && (!onf)) {
        command(0b10000000);
        sprintf(buf, "%04d/%02d/%02d      ", year, month, date);
        cssp_lcd_print(buf);
        command(0b11000000);
        sprintf(buf, "%s %02d:%02d:%02d     ", AM?"AM":"PM", hour, min, sec);
        cssp_lcd_print(buf);
    }
    else if((item == 5) && (!onf)) {
        command(0b10000000);
        sprintf(buf, "%04d/%02d/%02d %s  ", year, month, date, clock_findDow(dow));
        cssp_lcd_print(buf);
        command(0b11000000);
        sprintf(buf, "     :%02d:%02d     ", min, sec);
        cssp_lcd_print(buf);
    }
    else if((item == 6) && (!onf)) {
        command(0b10000000);
        sprintf(buf, "%04d/%02d/%02d %s  ", year, month, date, clock_findDow(dow));
        cssp_lcd_print(buf);
        command(0b11000000);
        sprintf(buf, "%s %02d:  :%02d     ", AM?"AM":"PM", hour, sec);
        cssp_lcd_print(buf);
    }
    else if((item == 7) && (!onf)) {
        command(0b10000000);
        sprintf(buf, "%04d/%02d/%02d %s  ", year, month, date, clock_findDow(dow));
        cssp_lcd_print(buf);
        command(0b11000000);
        sprintf(buf, "%s %02d:%02d:       ", AM?"AM":"PM", hour, min);
        cssp_lcd_print(buf);
    }
    else {
        command(0b10000000);
        sprintf(buf, "%04d/%02d/%02d %s  ", year, month, date, clock_findDow(dow));
        cssp_lcd_print(buf);
        command(0b11000000);
        sprintf(buf, "%s %02d:%02d:%02d     ", AM?"AM":"PM", hour, min, sec);
        cssp_lcd_print(buf);
    }
}

void clock_timeSetting_decrese(uint8_t item) {
    if(item==7)
        sec = 0;
    else if(item == 6) {
        if(min == 0)
            min = 59;
        else
            min--;
    }        
    else if(item == 5) {
        if(hour == 1) {
            if(AM)
                AM = false;
            else
                AM = true;
            hour = 12;
        }
        else
            hour--;
    }
    else if((item == 4) && (--dow == 0))
        dow = 7;
    else if(item == 3) {
        if((month == 2) && clock_isYoon(year) && (date == 1))
            date = 29;
        else if((month == 2) && !clock_isYoon(year) && (date == 1))
            date = 28;
        else if((month == 1||month == 3||month == 5||month == 7||month == 8||month == 10||month == 12) && (date == 1))
            date = 31;
        else if((month == 4||month == 6||month == 9||month == 11) && (date == 1))
            date = 30;
        else
            date--;
    }
    else if((item == 2) && (--month == 0))
        month = 12;
    else if((item == 1) && (--year == 1969))
        year = 9999;
    
    if(clock_isYoon(year) && (month == 2) && (date > 29))
        date = 29;
    else if(!clock_isYoon(year) && (month == 2) && (date > 28))
        date = 28;
    else if((month == 4||month == 6||month == 9||month == 11) && (date == 1) && (date > 30))
        date = 30;
}

void clock_timeSetting_increse(uint8_t item) {
    if((item==7))
        sec = 0;
    else if((item == 6) && (++min == 60))
        min = 0;
    else if((item == 5) && (++hour == 13)) {
        if(AM)
            AM = false;
        else
            AM = true;
        hour = 1;
    }
    else if((item == 4) && (++dow == 8))
        dow = 1;
    else if((item == 3) && (month == 2) && clock_isYoon(year) && (++date == 30))
        date = 1;
    else if((item == 3) && (month == 2) && !clock_isYoon(year) && (++date == 29))
        date = 1;
    else if((item == 3) && ((month == 1||month == 3||month == 5||month == 7||month == 8||month == 10||month == 12)) && (++date == 32))
        date = 1;
    else if((item == 3) && ((month == 4||month == 6||month == 9||month == 11)) && (++date == 31))
        date = 1;
    else if((item == 2) && (++month == 13))
        month = 1;
    else if((item == 1) && (++year == 10000))
        year = 1970;
}

void clock_timeSetting_nextItem() {
    item++;
    if(item==8)
        item = 1;
}

void clock_timeSetting_do(void) { 
    bool out1 = false, out2 = false, m_isbutton = false;
    bool out3 = false, out4 = false, m_isbutton1 = false;
    bool print = false;
    
    NRF_RTC2->TASKS_START = 1;

    while(1) {
        if(NRF_RTC2->EVENTS_COMPARE[0]--) {
            clock_timeSetting_print(item, print);
            NRF_RTC2->TASKS_CLEAR = 1;
            
            if(print)
                print = false;
            else if(!print)
                print = true;       
        }


        if(!cssp_gpio_read(13))
            break;
        else if(!cssp_gpio_read(14)) {
            NRF_TIMER4->CC[0] = 16000000 / pow(2, NRF_TIMER4->PRESCALER) * (300 * 1000) / 1000000;
    
            while(1) {
                if(isbutton) {
                    stillbutton = true;
                }

                if((!cssp_gpio_read(14)) && (!isbutton)) {   
                    clock_timeSetting_increse(item);
                    clock_timeSetting_print(item, true);
                    NRF_RTC2->TASKS_CLEAR = 1;
                    isbutton = true;
                    NRF_TIMER4->TASKS_START = 1;
                    out1 = true;
                }
                else if(!cssp_gpio_read(14)) {
                    // do nothing
                }
                else {
                    if(isbutton) {
                        NRF_TIMER4->TASKS_STOP = 1;
                        NRF_TIMER4->TASKS_CLEAR = 1;
                        out2 = true;
                    }
                    isbutton = false;
                    stillbutton = false;
                }

                if(stillbutton) {
                    if(still) {
                        clock_timeSetting_increse(item);
                        clock_timeSetting_print(item, true);
                        NRF_RTC2->TASKS_CLEAR = 1;
                        still = false;
                    }
                }
        
                if(!isbutton) {
                    NRF_TIMER4->TASKS_STOP = 1;
                    NRF_TIMER4->TASKS_CLEAR = 1;
                    still = false;
                    out2 = true;
                }

                if(out1&out2)
                    break;
            }
            out1 = false;
            out2 = false;
        }
        else if(!cssp_gpio_read(16)) {
            NRF_TIMER4->CC[0] = 16000000 / pow(2, NRF_TIMER4->PRESCALER) * (300 * 1000) / 1000000;
    
            while(1) {
                if(isbutton) {
                    stillbutton = true;
                }

                if((!cssp_gpio_read(16)) && (!isbutton)) {   
                    clock_timeSetting_decrese(item);
                    clock_timeSetting_print(item, true);
                    NRF_RTC2->TASKS_CLEAR = 1;
                    isbutton = true;
                    NRF_TIMER4->TASKS_START = 1;
                    out1 = true;
                }
                else if(!cssp_gpio_read(16)) {
                    // do nothing
                }
                else {
                    if(isbutton) {
                        NRF_TIMER4->TASKS_STOP = 1;
                        NRF_TIMER4->TASKS_CLEAR = 1;
                        out2 = true;
                    }
                    isbutton = false;
                    stillbutton = false;
                }

                if(stillbutton) {
                    if(still) {
                        clock_timeSetting_decrese(item);
                        clock_timeSetting_print(item, true);
                        NRF_RTC2->TASKS_CLEAR = 1;
                        still = false;
                    }
                }
        
                if(!isbutton) {
                    NRF_TIMER4->TASKS_STOP = 1;
                    NRF_TIMER4->TASKS_CLEAR = 1;
                    still = false;
                    out2 = true;
                }

                if(out1&out2)
                    break;
            }
            out1 = false;
            out2 = false;
        }
        else if(!cssp_gpio_read(15)) {
            while(1) {
                if((!cssp_gpio_read(15)) && (!m_isbutton1)) {   
                    clock_timeSetting_nextItem();
                    m_isbutton1 = true;
                    out3 = true;
                }
                else if(!cssp_gpio_read(15)) {
                    // do nothing
                }
                else {
                    m_isbutton1 = false;
                    if(out3)
                        out4 = true;
                }
        
                if(out3&out4)
                    break;
            }
            out3 = false;
            out4 = false;
        }
    }

    NRF_RTC2->TASKS_STOP = 1;
    NRF_RTC2->TASKS_CLEAR = 1;
    
    if(!cssp_gpio_read(13)) {
        while(1) {
            if((!cssp_gpio_read(13))) {
                item = 1;
                clock_main();
                m_isbutton = true;
                out1 = true;
            }
            else {
                m_isbutton = false;
                if(out1)
                    out2 = true;
            }
        
            if(out1&out2)
                break;
        }
    }
}

void clock_stopWatch_print(bool nclear) {
    if(!nclear)
        cssp_lcd_clear();
    command(0b10000000);
    cssp_lcd_print("STOPWATCH");
    command(0b11000000);
    sprintf(buf, "%02d:%02d:%02d:%02d", s_hour, s_min, s_sec, s_ms);
    cssp_lcd_print(buf);
}

void clock_stopWatch_startnStop(void) {
    bool out1 = false, out2 = false, m_isbutton = false;

    while(1) {
        if((!cssp_gpio_read(14)) && (!m_isbutton)) {   
            if(!timerWork) {
                NRF_TIMER3->TASKS_START = 1;
                NRF_GPIOTE->TASKS_CLR[1] = 1;
                timerWork = true;
            }
            else if(timerWork) {
                NRF_TIMER3->TASKS_STOP = 1;
                NRF_GPIOTE->TASKS_SET[1] = 1;
                timerWork = false;
            }
            m_isbutton = true;
            out1 = true;
        }
        else if(!cssp_gpio_read(14)) {
            // do nothing
        }
        else {
            m_isbutton = false;
            if(out1)
                out2 = true;
        }

        clock_stopWatch_print(true);
        
        if(out1&out2)
            break;      
    }
}

void clock_stopWatch_do(void) {
    while(1) {
        if(!cssp_gpio_read(13))
            break;
        else if(!cssp_gpio_read(14)) {
            //use TIMER3
            clock_stopWatch_startnStop();
        }
        else if(!cssp_gpio_read(15) && !timerWork) {
            NRF_TIMER3->TASKS_CLEAR = 1;
            s_hour = 0;
            s_min = 0;
            s_sec = 0;
            s_ms = 0;
        }

        clock_stopWatch_print(true);
    }
    
    if(!cssp_gpio_read(13))
        clock_buttonMenu(13, clock_alarmSetting_print, clock_alarmSetting_do);
}

void clock_alarmSetting_print(uint8_t a_item, bool onf) {
    if((a_item == 1) && !onf) {
        command(0b10000000);
        cssp_lcd_print("ALARM SETTING   ");
        command(0b11000000);
        sprintf(buf, "     :%02d     %3s", a_min, alarm?"ON ":"OFF");
        cssp_lcd_print(buf);
    }
    else if((a_item == 2) && !onf) {
        command(0b10000000);
        cssp_lcd_print("ALARM SETTING   ");
        command(0b11000000);
        sprintf(buf, "%s %02d:       %3s", a_AM?"AM":"PM", a_hour, alarm?"ON ":"OFF");
        cssp_lcd_print(buf);
    }
    else if((a_item == 3) && !onf) {
        command(0b10000000);
        cssp_lcd_print("ALARM SETTING   ");
        command(0b11000000);
        sprintf(buf, "%s %02d:%02d        ", a_AM?"AM":"PM", a_hour, a_min);
        cssp_lcd_print(buf);
    }
    else {
        command(0b10000000);
        cssp_lcd_print("ALARM SETTING   ");
        command(0b11000000);
        sprintf(buf, "%s %02d:%02d     %3s", a_AM?"AM":"PM", a_hour, a_min, alarm?"ON ":"OFF");
        cssp_lcd_print(buf);
    }
}

void clock_alarmSetting_increse(uint8_t a_item) {
    if((a_item == 1) && (++a_hour == 13)) {
        a_hour = 1;
        if(a_AM)
            a_AM = false;
        else if(!a_AM)
            a_AM = true;
    }
    else if((a_item == 2) && (++a_min == 60)) {
        a_min = 0;
    }
    else if((a_item == 3) && (alarm))
        alarm = false;
    else if((a_item == 3) && (!alarm))
        alarm = true;   
}

void clock_alarmSetting_decrese(uint8_t a_item) {
    if((a_item == 1) && (--a_hour == 0)) {
        a_hour = 12;
        if(a_AM)
            a_AM = false;
        else if(!a_AM)
            a_AM = true;
    }
    else if((a_item == 2) && (--a_min == 255))
        a_min = 59;
    else if((a_item == 3) && (alarm))
        alarm = false;
    else if((a_item == 3) && (!alarm))
        alarm = true;
}

void clock_alarmSetting_nextItem(void) {
    if(++a_item == 4)
        a_item = 1;
}

void clock_alarmSetting_do(void) {
    bool out1 = false, out2 = false, m_isbutton = false;
    bool out3 = false, out4 = false, m_isbutton1 = false;
    bool print = true;
    
    NRF_RTC2->TASKS_START = 1;

    while(1) {
        if(NRF_RTC2->EVENTS_COMPARE[0]--) {
            clock_alarmSetting_print(a_item, print);
            NRF_RTC2->TASKS_CLEAR = 1;
            
            if(print)
                print = false;
            else if(!print)
                print = true;       
        }

        if(!cssp_gpio_read(13))
            break;
        else if(!cssp_gpio_read(14)) {
            NRF_TIMER4->CC[0] = 16000000 / pow(2, NRF_TIMER4->PRESCALER) * (300 * 1000) / 1000000;
    
            while(1) {
                if(isbutton) {
                    stillbutton = true;
                }

                if((!cssp_gpio_read(14)) && (!isbutton)) {   
                    clock_alarmSetting_increse(a_item);
                    clock_alarmSetting_print(a_item, true);
                    isbutton = true;
                    NRF_TIMER4->TASKS_START = 1;
                    out1 = true;
                }
                else if(!cssp_gpio_read(14)) {
                    // do nothing
                }
                else {
                    if(isbutton) {
                        NRF_TIMER4->TASKS_STOP = 1;
                        NRF_TIMER4->TASKS_CLEAR = 1;
                        out2 = true;
                    }
                    isbutton = false;
                    stillbutton = false;
                }

                if(stillbutton) {
                    if(still) {
                        clock_alarmSetting_increse(a_item);
                        clock_alarmSetting_print(a_item, true);
                        still = false;
                    }
                }
        
                if(!isbutton) {
                    NRF_TIMER4->TASKS_STOP = 1;
                    NRF_TIMER4->TASKS_CLEAR = 1;
                    still = false;
                    out2 = true;
                }

                if(out1&out2)
                    break;
            }
            NRF_RTC2->TASKS_CLEAR = 1;
            out1 = false;
            out2 = false;
        }
        else if(!cssp_gpio_read(15)) {
            while(1) {
                if((!cssp_gpio_read(15)) && (!m_isbutton1)) {   
                    clock_alarmSetting_nextItem();
                    clock_alarmSetting_print(a_item, true);
                    m_isbutton1 = true;
                    out3 = true;
                }
                else if(!cssp_gpio_read(15)) {
                    // do nothing
                }
                else {
                    m_isbutton1 = false;
                    if(out3)
                        out4 = true;
                }
        
                if(out3&out4)
                    break;
            }
            NRF_RTC2->TASKS_CLEAR = 1;
            out3 = false;
            out4 = false;
        }
        else if(!cssp_gpio_read(16)) {
            NRF_TIMER4->CC[0] = 16000000 / pow(2, NRF_TIMER4->PRESCALER) * (300 * 1000) / 1000000;
    
            while(1) {
                if(isbutton) {
                    stillbutton = true;
                }

                if((!cssp_gpio_read(16)) && (!isbutton)) {   
                    clock_alarmSetting_decrese(a_item);
                    clock_alarmSetting_print(a_item, true);
                    isbutton = true;
                    NRF_TIMER4->TASKS_START = 1;
                    out1 = true;
                }
                else if(!cssp_gpio_read(16)) {
                    // do nothing
                }
                else {
                    if(isbutton) {
                        NRF_TIMER4->TASKS_STOP = 1;
                        NRF_TIMER4->TASKS_CLEAR = 1;
                        out2 = true;
                    }
                    isbutton = false;
                    stillbutton = false;
                }

                if(stillbutton) {
                    if(still) {
                        clock_alarmSetting_decrese(a_item);
                        clock_alarmSetting_print(a_item, true);
                        still = false;
                    }
                }
        
                if(!isbutton) {
                    NRF_TIMER4->TASKS_STOP = 1;
                    NRF_TIMER4->TASKS_CLEAR = 1;
                    still = false;
                    out2 = true;
                }

                if(out1&out2)
                    break;
            }
            NRF_RTC2->TASKS_CLEAR = 1;
            out1 = false;
            out2 = false;
        }
    }
    
    NRF_RTC2->TASKS_STOP = 1;
    NRF_RTC2->TASKS_CLEAR = 1;

    a_item = 1;

    if(alarm)
        NRF_GPIOTE->TASKS_CLR[0] = 1;
    else if(!alarm)
        NRF_GPIOTE->TASKS_SET[0] = 1;

    if(!cssp_gpio_read(13))
        clock_buttonMenu(13, clock_timeSetting_print, clock_timeSetting_do);
}
/*-----------------------------------------main----------------------------------------------*/

void main() {
    /* configuration RTC */
    NRF_CLOCK->TASKS_LFCLKSTART = 1;
    NRF_RTC0->INTENSET = RTC_INTENSET_TICK_Enabled << RTC_INTENSET_TICK_Pos;
    NRF_RTC0->EVTENSET = RTC_EVTENSET_TICK_Enabled << RTC_EVTENSET_TICK_Pos;
    NRF_RTC0->PRESCALER = 4095;
    
    NVIC_EnableIRQ(RTC0_IRQn);

    NRF_RTC2->INTENSET = RTC_INTENSET_TICK_Enabled << RTC_INTENSET_TICK_Pos;
    NRF_RTC2->INTENSET = RTC_INTENSET_COMPARE0_Enabled << RTC_INTENSET_COMPARE0_Pos;
    NRF_RTC2->EVTENSET = RTC_EVTENSET_TICK_Enabled << RTC_EVTENSET_TICK_Pos;
    NRF_RTC2->EVTENSET = RTC_EVTENSET_COMPARE0_Enabled << RTC_EVTENSET_COMPARE0_Pos;
    NRF_RTC2->CC[0] = 4;
    NRF_RTC2->PRESCALER = 4095;

    /* configuration TIMER */
    NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos;
    NRF_TIMER0->PRESCALER = 4; //1TICK = 1 Micro SEC
    NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
    NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
    NRF_TIMER0->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Disabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos;
    
    NRF_TIMER3->MODE = TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos;
    NRF_TIMER3->PRESCALER = 4; //1TICK = 1 Micro SEC
    NRF_TIMER3->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
    NRF_TIMER3->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
    NRF_TIMER3->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Disabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos;
    
    NRF_TIMER3->CC[0] = 16000000 / pow(2, NRF_TIMER3->PRESCALER) * 10000 / 1000000;

    NVIC_EnableIRQ(TIMER3_IRQn);

    NRF_TIMER4->MODE = TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos;
    NRF_TIMER4->PRESCALER = 4; //1TICK = 1 Micro SEC
    NRF_TIMER4->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
    NRF_TIMER4->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
    NRF_TIMER4->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Disabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos;
    
    NVIC_EnableIRQ(TIMER4_IRQn);

    /* configuration TWI */
    NRF_TWI0->PSELSCL = 27;
    NRF_TWI0->PSELSDA = 26;
    NRF_TWI0->FREQUENCY = TWI_FREQUENCY_FREQUENCY_K100 << TWI_FREQUENCY_FREQUENCY_Pos;
    NRF_TWI0->ADDRESS = 0x27;
    
    NRF_TWI0->SHORTS = TWI_SHORTS_BB_SUSPEND_Enabled << TWI_SHORTS_BB_SUSPEND_Pos;
    
    NRF_TWI0->INTENSET = TWI_INTENSET_BB_Enabled << TWI_INTENSET_BB_Pos;
    NRF_TWI0->INTENSET = TWI_INTENSET_TXDSENT_Enabled << TWI_INTENSET_TXDSENT_Pos;
    NRF_TWI0->INTENSET = TWI_INTENSET_RXDREADY_Enabled << TWI_INTENSET_RXDREADY_Pos;
    NRF_TWI0->INTENSET = TWI_INTENSET_ERROR_Enabled << TWI_INTENSET_ERROR_Pos;
    NRF_TWI0->INTENSET = TWI_INTENSET_SUSPENDED_Enabled << TWI_INTENSET_SUSPENDED_Pos;
    NRF_TWI0->INTENSET = TWI_INTENSET_STOPPED_Enabled << TWI_INTENSET_STOPPED_Pos;

    NRF_TWI0->ENABLE = TWI_ENABLE_ENABLE_Enabled << TWI_ENABLE_ENABLE_Pos;

    /* configuration GPIO */
    cssp_gpio_config(17, GPIO_DIR_OUTPUT, GPIO_INPUT_DISCONNECT, GPIO_PULL_PULLUP, GPIO_DRIVE_S0S1, GPIO_SENSE_DISABLED);

    cssp_gpio_config(13, GPIO_DIR_INPUT, GPIO_INPUT_CONNECT, GPIO_PULL_PULLUP, GPIO_DRIVE_S0S1, GPIO_SENSE_DISABLED);
    cssp_gpio_config(14, GPIO_DIR_INPUT, GPIO_INPUT_CONNECT, GPIO_PULL_PULLUP, GPIO_DRIVE_S0S1, GPIO_SENSE_DISABLED);
    cssp_gpio_config(15, GPIO_DIR_INPUT, GPIO_INPUT_CONNECT, GPIO_PULL_PULLUP, GPIO_DRIVE_S0S1, GPIO_SENSE_DISABLED);
    cssp_gpio_config(16, GPIO_DIR_INPUT, GPIO_INPUT_CONNECT, GPIO_PULL_PULLUP, GPIO_DRIVE_S0S1, GPIO_SENSE_DISABLED);
    
    /* configuration GPIOTE */
    NRF_GPIOTE->CONFIG[0] = ((GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos)
                            |(GPIOTE_CONFIG_OUTINIT_High << GPIOTE_CONFIG_OUTINIT_Pos)
                            |(GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos)
                            |(20 << GPIOTE_CONFIG_PSEL_Pos));

    NRF_GPIOTE->CONFIG[1] = ((GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos)
                            |(GPIOTE_CONFIG_OUTINIT_High << GPIOTE_CONFIG_OUTINIT_Pos)
                            |(GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos)
                            |(19 << GPIOTE_CONFIG_PSEL_Pos));

    /* do something */
    lcd_begin();
    alarm = false;

    NRF_RTC0->TASKS_START = 1;

    command(0b00001100);
    command(0b00010100);

    command(0b10000000); //set cursor position
    cssp_lcd_print("9999/99/99 ??? A");

    command(0b11000000); //set cursor position
    cssp_lcd_print("AM 99:99:99");
    
    bool m_isbutton1 = false, out3 = false, out4 = false, loop = false;;

    while(1) {
        clock_main();
        if(!cssp_gpio_read(13))
            clock_buttonMenu(13, clock_stopWatch_print, clock_stopWatch_do);

        if(alarmCheck) {
            alarmCheck = false;
            command(0b10000000);
            sprintf(buf, "ALARM ALARM ALAR");
            cssp_lcd_print(buf);
            command(0b11000000);
            sprintf(buf, "M ALARM ALARM AL");
            cssp_lcd_print(buf);
            while(1) {
                if(!cssp_gpio_read(13) || !cssp_gpio_read(14) || !cssp_gpio_read(15) || !cssp_gpio_read(16)) {
                    while(1) {
                        loop = true;
                        if((!cssp_gpio_read(13) || !cssp_gpio_read(14) || !cssp_gpio_read(15) || !cssp_gpio_read(16)) && (!m_isbutton1)) {   
                            m_isbutton1 = true;
                            out3 = true;
                        }
                        else if(!cssp_gpio_read(13) || !cssp_gpio_read(14) || !cssp_gpio_read(15) || !cssp_gpio_read(16)) {
                            // do nothing
                        }
                        else {
                            m_isbutton1 = false;
                            if(out3)
                                out4 = true;
                        }
        
                        if(out3&out4)
                            break;
                    }
                    out3 = false;
                    out4 = false;
                }

                if(loop)
                    break;
            }
            loop = false;
        }
    }   
}