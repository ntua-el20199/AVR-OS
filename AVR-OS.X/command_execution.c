#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include <avr/eeprom.h>

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
    char *arg = strtok(NULL, " ");      // Second token is the argument (if any)

    execute_command(cmd_name, arg);
}

// Process received commands
void execute_command(char *cmd, char *arg)
{
    char readBuffer[50];
    if (strncmp(cmd, "setuser ", 4) == 0)
    {
        usart_send_string("Setting user: ");
        usart_send_string(arg);
        usart_send_string("\n");
        write_eeprom(0x00, arg);
        usart_send_string("User set successfully\n");
    }
    
    else if (strncmp(cmd, "getuser ", 4) == 0)
    {
        read_eeprom(0x00, readBuffer, sizeof(readBuffer));
        usart_send_string(readBuffer);
    }
    
    else if (strncmp(cmd, "setpass", 4) == 0)
    {
        write_eeprom(0x20, arg);
        usart_send_string("Password set successfully\n");
    }
    
    else if (strncmp(cmd, "getpass", 4) == 0)
    {
        read_eeprom(0x20, readBuffer, sizeof(readBuffer));
        usart_send_string(readBuffer);
    }

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
