/*
 * --------------------------------------------------------------------
 * SMART STREET LIGHT CONTROLLER WITH SUNRISE & SUNSET LOGGING
 * --------------------------------------------------------------------
 * Microcontroller : PIC16F877A
 * Clock Frequency : 4 MHz
 *
 * Features:
 *  - Day/Night detection using LDR
 *  - Motion-based lighting using PIR
 *  - Software RTC using Timer2 interrupt
 *  - Sunrise & sunset time logging based on LDR transitions
 *  - 16x2 LCD status display
 * --------------------------------------------------------------------
 */

#include <xc.h>

/* ================= CONFIGURATION BITS ================= */
#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = OFF
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

#define _XTAL_FREQ 4000000

/* ================= LCD PIN DEFINITIONS ================= */
#define LCD_RS RD0
#define LCD_EN RD3
#define LCD_D4 RD4
#define LCD_D5 RD5
#define LCD_D6 RD6
#define LCD_D7 RD7

/* ================= TIME VARIABLES ================= */
volatile unsigned char seconds = 0;
volatile unsigned char minutes = 0;
volatile unsigned char hours   = 22;

/* ================= SUNRISE & SUNSET VARIABLES ================= */
volatile unsigned char sunrise_h = 0, sunrise_m = 0;
volatile unsigned char sunset_h  = 0, sunset_m  = 0;

unsigned char prevLDRState = 0;

/* ================= FUNCTION PROTOTYPES ================= */
void System_Init(void);
void Port_Init(void);
void Timer2_Init(void);
void Interrupt_Init(void);

void LCD_Init(void);
void LCD_Command(unsigned char);
void LCD_Data(unsigned char);
void LCD_String(const char*);
void LCD_Clear(void);
void LCD_SetCursor(unsigned char, unsigned char);

void Update_Display(unsigned char, unsigned char, unsigned char);
void Show_Sunrise_Sunset(void);
void LCD_Print_Time(unsigned char, unsigned char);

/* ================= TIMER2 ISR ================= */
void __interrupt() ISR(void)
{
    if(PIR1bits.TMR2IF)
    {
        PIR1bits.TMR2IF = 0;

        seconds++;
        if(seconds >= 60)
        {
            seconds = 0;
            minutes++;

            if(minutes >= 60)
            {
                minutes = 0;
                hours++;
                if(hours >= 24)
                    hours = 0;
            }
        }
    }
}

/* ================= MAIN PROGRAM ================= */
void main(void)
{
    unsigned char isNight;
    unsigned char motion;
    unsigned char brightness;

    System_Init();

    LCD_Clear();
    LCD_String("Street Light");
    LCD_SetCursor(2,1);
    LCD_String("Controller");
    __delay_ms(2000);
    LCD_Clear();

    prevLDRState = PORTBbits.RB0;

    while(1)
    {
        isNight = PORTBbits.RB0;
        motion  = PORTDbits.RD2;

        /* Sunrise & Sunset detection based on LDR */
        if(prevLDRState == 1 && isNight == 0)
        {
            sunrise_h = hours;
            sunrise_m = minutes;
            Show_Sunrise_Sunset();
        }
        else if(prevLDRState == 0 && isNight == 1)
        {
            sunset_h = hours;
            sunset_m = minutes;
            Show_Sunrise_Sunset();
        }
        prevLDRState = isNight;

        /* Light control logic */
        if(isNight == 0)
        {
            PORTCbits.RC1 = 0;
            brightness = 0;
        }
        else
        {
            if(motion)
            {
                PORTCbits.RC1 = 1;
                brightness = 2;
            }
            else
            {
                PORTCbits.RC1 = 0;
                brightness = 0;
            }
        }

        Update_Display(isNight, motion, brightness);
        __delay_ms(150);
    }
}

/* ================= INITIALIZATION ================= */
void System_Init(void)
{
    Port_Init();
    Timer2_Init();
    Interrupt_Init();
    LCD_Init();
}

void Port_Init(void)
{
    TRISBbits.TRISB0 = 1;
    TRISCbits.TRISC1 = 0;
    TRISD = 0x04;

    PORTCbits.RC1 = 0;
    PORTD = 0x00;
    OPTION_REGbits.nRBPU = 0;
}

void Timer2_Init(void)
{
    T2CON = 0x00;
    TMR2 = 0;
    PR2 = 249;
    T2CONbits.T2CKPS0 = 1;
    T2CONbits.T2CKPS1 = 1;
    T2CONbits.TMR2ON = 1;
}

void Interrupt_Init(void)
{
    PIE1bits.TMR2IE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
}

/* ================= LCD FUNCTIONS ================= */
void LCD_Init(void)
{
    __delay_ms(20);
    LCD_Command(0x02);
    LCD_Command(0x28);
    LCD_Command(0x0C);
    LCD_Command(0x06);
    LCD_Command(0x01);
}

void LCD_Command(unsigned char cmd)
{
    LCD_RS = 0;

    LCD_D4 = (cmd >> 4) & 1;
    LCD_D5 = (cmd >> 5) & 1;
    LCD_D6 = (cmd >> 6) & 1;
    LCD_D7 = (cmd >> 7) & 1;
    LCD_EN = 1; __delay_us(1); LCD_EN = 0;

    LCD_D4 = cmd & 1;
    LCD_D5 = (cmd >> 1) & 1;
    LCD_D6 = (cmd >> 2) & 1;
    LCD_D7 = (cmd >> 3) & 1;
    LCD_EN = 1; __delay_us(1); LCD_EN = 0;

    __delay_ms(2);
}

void LCD_Data(unsigned char dat)
{
    LCD_RS = 1;

    LCD_D4 = (dat >> 4) & 1;
    LCD_D5 = (dat >> 5) & 1;
    LCD_D6 = (dat >> 6) & 1;
    LCD_D7 = (dat >> 7) & 1;
    LCD_EN = 1; __delay_us(1); LCD_EN = 0;

    LCD_D4 = dat & 1;
    LCD_D5 = (dat >> 1) & 1;
    LCD_D6 = (dat >> 2) & 1;
    LCD_D7 = (dat >> 3) & 1;
    LCD_EN = 1; __delay_us(1); LCD_EN = 0;

    __delay_ms(2);
}

void LCD_String(const char* str)
{
    while(*str)
        LCD_Data(*str++);
}

void LCD_Clear(void)
{
    LCD_Command(0x01);
    __delay_ms(2);
}

void LCD_SetCursor(unsigned char row, unsigned char col)
{
    LCD_Command((row == 1 ? 0x80 : 0xC0) + col - 1);
}

/* ================= DISPLAY FUNCTIONS ================= */
void Update_Display(unsigned char isNight,
                    unsigned char motion,
                    unsigned char level)
{
    LCD_SetCursor(1,1);
    LCD_String(isNight ? "Night " : "Day   ");
    LCD_String("M:");
    LCD_String(motion ? "YES " : "NO  ");

    LCD_SetCursor(2,1);
    LCD_String("Light:");
    LCD_String(level ? "ON   " : "OFF  ");
}

void LCD_Print_Time(unsigned char h, unsigned char m)
{
    LCD_Data((h/10) + '0');
    LCD_Data((h%10) + '0');
    LCD_Data(':');
    LCD_Data((m/10) + '0');
    LCD_Data((m%10) + '0');
}

void Show_Sunrise_Sunset(void)
{
    LCD_Clear();

    LCD_SetCursor(1,1);
    LCD_String("Sunrise:");
    LCD_Print_Time(sunrise_h, sunrise_m);

    LCD_SetCursor(2,1);
    LCD_String("Sunset :");
    LCD_Print_Time(sunset_h, sunset_m);

    __delay_ms(5000);
    LCD_Clear();
}
