#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>

#include "UART_setup.h"
#include "command_execution.h"
#include "eeprom.h"
#include "setup_LCD_PEX.h"
#include "setup_TWI.h"

#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define MAX_CMD_LEN 32

void setup() {  
    twi_init();
    _delay_ms(100);
    PCA9555_0_write(REG_CONFIGURATION_0, 0x00);
    lcd_init();
    _delay_ms(100);       // Delay 100 mS
    lcd_clear_display();
    _delay_ms(100);
    usart_init(MYUBRR);
    usart_send_string("Welcome to AVR-OS\n");
    usart_send_string("Login\n");
}

int main(void)
{
    setup();
    char *cmd;
    login();
    while (1)
    {
        cmd = NULL;
        while (cmd == NULL)  // Keep calling until a full command is received
        {
            cmd = uart_receive_command();
        }
        parse_command(cmd);
    }
}

