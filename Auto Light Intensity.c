/*
 * Smart Street Light Controller
 * PIC16F877A based system using LDR, PIR, LED, and 16x2 LCD
 * Designed for Proteus simulation and real hardware
 */

#include <xc.h>

/* ================= CONFIGURATION BITS ================= */
#pragma config FOSC = HS      // High-speed external crystal
#pragma config WDTE = OFF     // Watchdog Timer disabled
#pragma config PWRTE = ON     // Power-up Timer enabled
#pragma config BOREN = OFF    // Brown-out Reset disabled
#pragma config LVP = OFF     // Low Voltage Programming disabled
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

/* System oscillator frequency (required for delays) */
#define _XTAL_FREQ 20000000

/* ================= LCD PIN DEFINITIONS ================= */
#define LCD_RS RD0
#define LCD_EN RD3
#define LCD_D4 RD4
#define LCD_D5 RD5
#define LCD_D6 RD6
#define LCD_D7 RD7

/* ================= GLOBAL TIME VARIABLES ================= */
volatile unsigned char seconds = 0;
volatile unsigned char minutes = 0;
volatile unsigned char hours   = 22;

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

/* ================= TIMER2 INTERRUPT SERVICE ROUTINE ================= */
void __interrupt() ISR(void)
{
    if(PIR1bits.TMR2IF)
    {
        PIR1bits.TMR2IF = 0;   // Clear Timer2 interrupt flag

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

    while(1)
    {
        /* Sensor inputs */
        isNight = PORTBbits.RB0;   // LDR output
        motion  = PORTDbits.RD2;   // PIR output

        /* Day mode */
        if(isNight == 0)
        {
            PORTCbits.RC1 = 0;
            brightness = 0;
        }
        /* Night mode */
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

/* ================= SYSTEM INITIALIZATION ================= */
void System_Init(void)
{
    Port_Init();
    Timer2_Init();
    Interrupt_Init();
    LCD_Init();
}

/* ================= I/O PORT CONFIGURATION ================= */
void Port_Init(void)
{
    TRISBbits.TRISB0 = 1;   // LDR input
    TRISCbits.TRISC1 = 0;   // LED output
    TRISD = 0x04;           // RD2 input, others LCD outputs

    PORTCbits.RC1 = 0;
    PORTD = 0x00;

    OPTION_REGbits.nRBPU = 0;   // Enable PORTB pull-ups
}

/* ================= TIMER2 CONFIGURATION ================= */
void Timer2_Init(void)
{
    T2CON = 0x00;
    TMR2 = 0;
    PR2 = 249;                  // Period register

    T2CONbits.T2CKPS0 = 1;      // Prescaler 1:16
    T2CONbits.T2CKPS1 = 1;
    T2CONbits.TMR2ON  = 1;      // Enable Timer2
}

/* ================= INTERRUPT CONFIGURATION ================= */
void Interrupt_Init(void)
{
    INTCON = 0x00;
    PIE1bits.TMR2IE = 1;    // Enable Timer2 interrupt
    INTCONbits.PEIE = 1;    // Peripheral interrupts
    INTCONbits.GIE  = 1;    // Global interrupts
}

/* ================= LCD CONTROL FUNCTIONS ================= */
void LCD_Init(void)
{
    __delay_ms(20);
    LCD_Command(0x02);  // 4-bit mode
    LCD_Command(0x28);  // 2-line display
    LCD_Command(0x0C);  // Display ON
    LCD_Command(0x06);  // Cursor increment
    LCD_Command(0x01);  // Clear display
    __delay_ms(2);
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

/* ================= LCD STATUS DISPLAY ================= */
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
    LCD_String(level == 2 ? "ON   " : "OFF  ");
}
