#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <stdlib.h>

#include "UART_setup.h"
#include "eeprom.h"
#include "setup_LCD_PEX.h"

#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define MAX_CMD_LEN 32

#define USERNAME_ADDR  0x00  // EEPROM address for username
#define PASSWORD_ADDR  0x10  // EEPROM address for password
#define INIT_CHECK_ADDR 0x20 // EEPROM address for initialization check
#define MAGIC_VALUE 0xAA      // Unique byte to mark EEPROM as initialized

void execute_command(char *cmd, char **arg);
void login();
void print_to_lcd(char *cmd);

// Break up received string into tokens
void parse_command(char *cmd)
{
    usart_send_string("Parsing command: ");
    usart_send_string(cmd);
    usart_send_string("\n");

    char *cmd_name = strtok(cmd, " ");  // First token is the command name
    char *arg[2] = {NULL, NULL};        // Allocate space for two arguments, initialized to NULL
    
    if (strncmp(cmd_name, "lcd", sizeof(cmd_name)) == 0) {
        print_to_lcd(cmd);
    } else {
        // Otherwise, tokenize normally
        arg[0] = strtok(NULL, " "); // First argument
        arg[1] = strtok(NULL, " "); // Second argument (for gpiow, etc.)
        execute_command(cmd_name, arg);
    }
    memset(cmd, 0, strlen(cmd));
}

void print_to_lcd(char *cmd)
{
    lcd_clear_display();
    lcd_set_cursor(0, 0);

    int i = 4; // Start after "lcd "
    
    // Print characters until we hit the null terminator
    while (cmd[i] != '\0') {
        lcd_data(cmd[i]);
        i++;
        if(i == 20)
            lcd_set_cursor(1, 0);
    }
}

// Process received commands
void execute_command(char *cmd, char **arg)
{
    char readBuffer[50];
    if (strncmp(cmd, "setuser", 7) == 0)
    {
        write_eeprom(0x00, arg[0]);
        usart_send_string("User set successfully.\n");
    }
    else if (strncmp(cmd, "getuser", 7) == 0)
    {
        read_eeprom(0x00, readBuffer, sizeof(readBuffer));
        usart_send_string(readBuffer);
    }
    else if (strncmp(cmd, "setpass", 7) == 0)
    {
        write_eeprom(0x20, arg[0]);
        usart_send_string("Password set successfully.\n");
    }
    else if (strncmp(cmd, "getpass", 7) == 0)
    {
        read_eeprom(0x20, readBuffer, sizeof(readBuffer));
        usart_send_string(readBuffer);
    }
    else if (strcmp(cmd, "gpiow") == 0)
    {
        uint8_t input = 0x00;
        char *value = arg[1];
        if (value == NULL) input = 0x00;
        else {
            if (value[0] != '0' || value[1] != 'x' || value[2] == '\0') {
                usart_send_string("Error: Invalid format. Expected 0xHH\n");
            }
            else {
                input = (uint8_t) strtoul(value, NULL, 16);
            }
        }

        if (arg[0] != NULL) {
            /*Check which port is to write to*/
            if (strcmp(arg[0], "PORTB") == 0) {
                DDRB = 0xFF;
                PORTB = input;
            }
            else if (strcmp(arg[0], "PORTC") == 0) {
                DDRC = 0xFF;
                PORTC = input;
            }
            else if (strcmp(arg[0], "PORTD") == 0) {
                DDRD = 0xFF;
                PORTD = input;
            }
            else {/*Wrong port*/
                usart_send_string("Invalid port\n");
            }
        }
        else {
            /*If argument is vacant we tell the user to provide it*/
            usart_send_string("Port needs to be specified\n");
        }
    }
    else if (strcmp(cmd, "gpior") == 0)
    {
        uint8_t output = 0x00;
        char printable[20];
        if (arg[0] != NULL) {
            /*Check which port is to read from*/
            if (strcmp(arg[0], "PORTB") == 0) {
                DDRB = 0x00;
                PORTB = 0xFF;
                output = (~PINB & 0b00111111);
                sprintf(printable, "PORTB = %d\n", output);
                usart_send_string(printable);
            }
            else if (strcmp(arg[0], "PORTC") == 0) {
                DDRC = 0x00;
                PORTC = 0xFF;
                output = (~PINC & 0b00111111);
                sprintf(printable, "PORTC = %d\n", output);
                usart_send_string(printable);
            }
            else if (strcmp(arg[0], "PORTD") == 0) {
                DDRD = 0x00;
                PORTD = 0xFF;
                output = ~PIND;
                sprintf(printable, "PORTD = %d\n", output);
                usart_send_string(printable);
            }
            else {/*Wrong port*/
                usart_send_string("Invalid port\n");
            }
        }
        else {
            /*If argument is vacant we tell the user to provide it*/
            usart_send_string("Port needs to be specified\n");
        }
    }
    else if (strcmp(cmd, "logout") == 0) {
        usart_send_string("Logged out successfully.\n");
        login();
    }
    else {
        usart_send_string("Unknown command\n");
    }
    return;
}

void login()
{
    char *cmd = NULL;
    char readBuffer[50];
    int attempts = 0;
    
    usart_send_string("Enter username: ");
    
    while(1) {
        cmd = NULL;
        
        while (cmd == NULL)  // Keep calling until a full command is received
        {
            cmd = uart_receive_command();
        }
        
        read_eeprom(0x00, readBuffer, sizeof(readBuffer));
        
        if (strncmp(cmd, readBuffer, 10) == 0) {
            usart_send_string("Success\n");
            break;
        }
        else 
            usart_send_string("Wrong username. Try again.\n");
        
        attempts++;
        
        if(attempts == 3){
            usart_send_string("Too many attempts. Try again in 30 seconds");
            _delay_ms(3000);
            attempts = 0;
        }
    }

    usart_send_string("Enter password: ");
    
    while(1) {
        cmd = NULL;
        
        while (cmd == NULL)  // Keep calling until a full command is received
        {
            cmd = uart_receive_command();
        }
        
        read_eeprom(0x20, readBuffer, sizeof(readBuffer)); 
        
        if (strncmp(cmd, readBuffer, 10) == 0) {
            usart_send_string("Success\n");
            break;
        }
        else 
            usart_send_string("Wrong password. Try again.\n");
        
        attempts++;
        
        if(attempts == 3){
            usart_send_string("Too many attempts. Try again in 30 seconds");
            _delay_ms(10);
            attempts = 0;
        }
    }
}
