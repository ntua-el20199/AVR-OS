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
#include "setup_OWI.h"


#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define MAX_CMD_LEN 32

#define USERNAME_ADDR  0x00  // EEPROM address for username
#define PASSWORD_ADDR  0x10  // EEPROM address for password
#define INIT_CHECK_ADDR 0x20 // EEPROM address for initialization check
#define MAGIC_VALUE 0xAA      // Unique byte to mark EEPROM as initialized

uint8_t temp_l, temp_h;

void execute_command(char *cmd, char **arg);
void login();
void print_to_lcd(char *cmd);
double measure_temp();
double read_adc();

// Break up received string into tokens
void parse_command(char *cmd)
{
    //usart_send_string("Parsing command: ");
    //usart_send_string(cmd);
    //usart_send_string("\n\r");

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
    if (strcmp(cmd, "setuser") == 0)
    {
        write_eeprom(0x00, arg[0]);
        usart_send_string("User set successfully.\n\r");
    }
    else if (strcmp(cmd, "getuser") == 0)
    {
        read_eeprom(0x00, readBuffer, sizeof(readBuffer));
        usart_send_string(readBuffer);
        usart_send_string("\n\r");
    }
    else if (strcmp(cmd, "setpass") == 0)
    {
        write_eeprom(0x20, arg[0]);
        usart_send_string("Password set successfully.\n\r");
    }
    else if (strcmp(cmd, "getpass") == 0)
    {
        read_eeprom(0x20, readBuffer, sizeof(readBuffer));
        usart_send_string(readBuffer);
        usart_send_string("\n\r");
    }
    else if (strcmp(cmd, "gpiow") == 0)
    {
        uint8_t input = 0x00;
        char *value = arg[1];
        if (value == NULL) input = 0x00;
        else {
            if (value[0] != '0' || value[1] != 'x' || value[2] == '\0') {
                usart_send_string("Error: Invalid format. Expected 0xhh\n\r");
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
                usart_send_string("Invalid port\n\r");
            }
        }
        else {
            /*If argument is vacant we tell the user to provide it*/
            usart_send_string("Port needs to be specified\n\r");
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
                sprintf(printable, "PORTB = %d\n\r", output);
                usart_send_string(printable);
            }
            else if (strcmp(arg[0], "PORTC") == 0) {
                DDRC = 0x00;
                PORTC = 0xFF;
                output = (~PINC & 0b00111111);
                sprintf(printable, "PORTC = %d\n\r", output);
                usart_send_string(printable);
            }
            else if (strcmp(arg[0], "PORTD") == 0) {
                DDRD = 0x00;
                PORTD = 0xFF;
                output = (~PIND & 0b11111100);
                sprintf(printable, "PORTD = %d\n\r", output);
                usart_send_string(printable);
            }
            else {/*Wrong port*/
                usart_send_string("Invalid port\n\r");
            }
        }
        else {
            /*If argument is vacant we tell the user to provide it*/
            usart_send_string("Port needs to be specified\n\r");
        }
    }
    else if (strcmp(cmd, "logout") == 0) {
        usart_send_string("Logged out successfully.\n\r");
        login();
    }
    else if (strcmp(cmd, "temperature") == 0) {
        double temp = measure_temp();
        int temp_int = (int) temp;            // Integer part
        int temp_frac = (int)((temp - temp_int) * 10);  // First decimal place

        char temp_out[10];  // Buffer large enough
        sprintf(temp_out, "%d.%d", temp_int, temp_frac);  // Format manually
        
        usart_send_string("Measured temperature: ");
        usart_send_string(temp_out);
        usart_send_string("\n\r");
    }
    else if (strcmp(cmd, "potentiometer") == 0) {
        double adc = read_adc();
        int adc_int = (int) adc;            // Integer part
        int adc_frac = (int)((adc - adc_int) * 10);  // First decimal place

        char adc_out[10];  // Buffer large enough
        sprintf(adc_out, "%d.%d", adc_int, adc_frac);  // Format manually
        usart_send_string("Potentiometer value: ");
        usart_send_string(adc_out);
        usart_send_string("\x25");
        usart_send_string("\n\r");
    }
    else {
        usart_send_string("Unknown command\n\r");
    }
    return;
}

void login()
{
    char cmd[MAX_CMD_LEN];  // Buffer for username and password
    char readBuffer[50];
    int attempts = 0;

    while (1) {
        while (1) {
            usart_send_string("Enter username: ");
            char *input = NULL;
            while (input == NULL) {  // Wait until a command is received
                input = uart_receive_command();
            }
            strncpy(cmd, input, MAX_CMD_LEN);
            cmd[MAX_CMD_LEN - 1] = '\0';  // Ensure null termination
            
            read_eeprom(0x00, readBuffer, sizeof(readBuffer));
            
            if (strncmp(cmd, readBuffer, 10) == 0) {
                usart_send_string("Success\n\r");
                break;  // Move to password entry
            } else {
                usart_send_string("Wrong username. Try again.\n\r");
            }
        }

        // Password loop (user gets 3 tries)
        while (attempts < 3) {
            usart_send_string("Enter password: ");
            
            char password[MAX_CMD_LEN];
            uint8_t i = 0;

            while (1) {
                char c = usart_receive();  // Receive a character

                if (c == '\r' || c == '\n') {  // If Enter is pressed
                    password[i] = '\0';  // Null-terminate
                    usart_send_string("\n\r");  // Move to the next line
                    break;
                } 
                else if (i < MAX_CMD_LEN - 1) {  
                    password[i++] = c;  // Store character but don't echo it
                }
            }

            // Compare entered password with stored password
            read_eeprom(0x20, readBuffer, sizeof(readBuffer));

            if (strncmp(password, readBuffer, 10) == 0) {
                usart_send_string("Success\n\r");
                return;  // Exit function after successful login
            } else {
                usart_send_string("Wrong password. Try again.\n\r");
                attempts++;
            }
        }

        // If user fails 3 times, enforce cooldown
        usart_send_string("Too many attempts. Try again in 30 seconds.\n\r");
        _delay_ms(3000);
        attempts = 0;  // Reset attempts
    }
}

double measure_temp()
{   
    int16_t temperature = 0;
    if(one_wire_reset()) {
        one_wire_transmit_byte(0xCC);
        one_wire_transmit_byte(0x44);
        while(!one_wire_receive_bit()) {
            //busy waiting
        }
        if(one_wire_reset()) {
            one_wire_transmit_byte(0xCC);
            one_wire_transmit_byte(0xBE);
            temp_l = one_wire_receive_byte();
            temp_h = one_wire_receive_byte();
        }
        else {
            temp_l = 0x00;
            temp_h = 0x80;
        }
    }
    else {
        temp_l = 0x00;
        temp_h = 0x80;
    }   
       
    temperature = (temp_h & 0b00000111) << 8;
    temperature |= temp_l;
    
    return ((double)temperature * 0.06);
}


double read_adc() 
{
    double adc;
    double output;
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));  
    adc = ADC;
    output = (adc * 100) / 1024;
    return output; 
}
