#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <stdlib.h>

#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define MAX_CMD_LEN 32

#define USERNAME_ADDR  0x00  // EEPROM address for username
#define PASSWORD_ADDR  0x10  // EEPROM address for password
#define INIT_CHECK_ADDR 0x20 // EEPROM address for initialization check
#define MAGIC_VALUE 0xAA      // Unique byte to mark EEPROM as initialized

// Break up received string into tokens
void parse_command(char *cmd)
{
    char *cmd_name = strtok(cmd, " ");  // First token is the command name
    char *arg[2] = {NULL, NULL};  // Allocate space for two arguments, initialized to NULL
    arg[0] = strtok(NULL, " ");      // Second token is the argument (if any)
    arg[1] = strtok(NULL, " ");      // Third token is the 2nd argument (only for gpiow)
    execute_command(cmd_name, arg);
}

// Process received commands
void execute_command(char *cmd, char **arg)
{
    char readBuffer[50];
    if (strncmp(cmd, "setuser ", 4) == 0)
    {
        usart_send_string("Setting user: ");
        usart_send_string(arg[0]);
        usart_send_string("\n");
        write_eeprom(0x00, arg[0]);
        usart_send_string("User set successfully\n");
    }
    
    else if (strncmp(cmd, "getuser ", 4) == 0)
    {
        read_eeprom(0x00, readBuffer, sizeof(readBuffer));
        usart_send_string(readBuffer);
    }
    
    else if (strncmp(cmd, "setpass", 4) == 0)
    {
        write_eeprom(0x20, arg[0]);
        usart_send_string("Password set successfully\n");
    }
    
    else if (strncmp(cmd, "getpass", 4) == 0)
    {
        read_eeprom(0x20, readBuffer, sizeof(readBuffer));
        usart_send_string(readBuffer);
    }
    
    else if (strcmp(cmd, "gpiow") == 0)
    {   
        uint8_t input;
        char *value = arg[1];
        if(value == NULL) input = 0x00;
        else {
            if (value[0] != '0' || value[1] != 'x' || value[2] == '\0') {
                usart_send_string("Error: Invalid format. Expected 0xHH\n");
            }
            else {
                input = (uint8_t) strtoul(value, NULL, 16);
            }
        }
        
        if(arg[0] != NULL) {
            /*Check which port is to write to*/
            if(strcmp(arg[0], "PORTB") == 0) {
                PORTB = input;
            }
            else if(strcmp(arg[0], "PORTC") == 0) {
                PORTC = input;
            }
            else if(strcmp(arg[0], "PORTD") == 0) {
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
    
    else if(strcmp(cmd, "gpior") == 0)
    {
        uint8_t output = 0x00;
        char printable[20];
        if(arg[0] != NULL) {
            /*Check which port is to write to*/
            if(strcmp(arg[0], "PORTB") == 0) {
                output = PORTB;
                sprintf(printable, "PORTB = %d\n", output);
                usart_send_string(printable);
            }
            else if(strcmp(arg[0], "PORTC") == 0) {
                output = PORTC;
                sprintf(printable, "PORTC = %d\n", output);
                usart_send_string(printable);
            }
            else if(strcmp(arg[0], "PORTD") == 0) {
                output = PORTD;
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
    else {
        usart_send_string("Uknown command\n");
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
        
        if (strncmp(cmd, readBuffer, sizeof(cmd)) == 0) {
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
        
        if (strncmp(cmd, readBuffer, sizeof(cmd)) == 0) {
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
