#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>

#include "UART_setup.h"
#include "command_execution.h"
#include "eeprom.h"

#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define MAX_CMD_LEN 32

int main(void)
{
    usart_init(MYUBRR);
    usart_send_string("Welcome to AVR-OS\n");

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

