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
    static uint8_t cursor_pos = 0;
    usart_transmit(c);  // Echo character back

    if (c == '\r' || c == '\n') // Enter key
    {
        if (i > 0)
        {
            command[i] = '\0'; // Null-terminate
            usart_send_string("\n"); // Move to next line
            i = 0; // Reset buffer for next command
            cursor_pos = 0; //Reset cursor position
            return command; // Return the full command
        }
        else
        {   usart_send_string("\n");
            return NULL; // Ignore empty command
        }
    }
    // Handle Backspace
    else if (c == 0x08 || c == 0x7f)
    {
        if (cursor_pos > 0)  // Only delete if cursor is not at start
        {   
            
            for (uint8_t j = cursor_pos - 1; j < i-1; j++)
            {   
                command[j] = command[j + 1];  // Shift characters left
            }
            
            command[cursor_pos - 1] = '\0';
            i--;
            cursor_pos--;
            command[i] = '\0';  // Maintain null-termination
            
            
        }
    }
    else if (c == '\x1B')  // Escape sequence start
    {
        char next = usart_receive();  // Read '['
        char direction = usart_receive();  // Read 'D' or 'C'
        
        if (next == '[')
        {
            if (direction == 'D' && cursor_pos > 0)  // Left Arrow
            {
                usart_send_string("\b");  // Move cursor left
                cursor_pos--;
            }
            else if (direction == 'C' && cursor_pos < i)  // Right Arrow
            {   
                usart_send_string("\x1B[C");
                cursor_pos++;
            }
        }
    }
    else if (i < MAX_CMD_LEN - 1)
    {
        // Insert at cursor position
        for (uint8_t j = i; j > cursor_pos; j--)
        {
            command[j] = command[j-1];  // Shift characters right
        }
        command[cursor_pos] = c;  // Insert new character
        i++;
        cursor_pos++;
        command[i] = '\0';  // Ensure null-termination     
    }

    return NULL; // Keep waiting for full command
}
