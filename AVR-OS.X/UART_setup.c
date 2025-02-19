#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>

#include "command_execution.h"

#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define MAX_CMD_LEN 32

char command[MAX_CMD_LEN];  
uint8_t i = 0;

// Initialize USART (Serial Communication)
void usart_init(unsigned int ubrr)
{
    UCSR0A = 0;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0C = (3 << UCSZ00);
}

// Send a single character via USART
void usart_transmit(uint8_t data)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

// Send a full string via USART
void usart_send_string(const char *s)
{
    while (*s)
    {
        usart_transmit(*s++);
    }
}

// Receive a single character via USART
uint8_t usart_receive()
{
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

char* uart_receive_command()
{
    static uint8_t i = 0;  // Keeps track of position
    char c = usart_receive();
    usart_transmit(c);  // Echo character back

    if (c == '\r' || c == '\n') // Enter key
    {
        if (i > 0)
        {
            command[i] = '\0'; // Null-terminate
            usart_send_string("\n"); // Move to next line
            i = 0; // Reset buffer for next command
            return command; // Return the full command
        }
        else
        {
            return NULL; // Ignore empty command
        }
    }
    else if (i < MAX_CMD_LEN - 1)
    {
        command[i++] = c; // Store character in buffer
    }

    return NULL; // Keep waiting for full command
}
