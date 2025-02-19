#include <avr/eeprom.h>
#include <string.h>

void write_eeprom(uint16_t addr, const char *data) 
{
    eeprom_write_block((const void *)data, (void *)addr, strlen(data) + 1);
}

void read_eeprom(uint16_t addr, char *buffer, size_t length) 
{
    eeprom_read_block((void *)buffer, (const void *)addr, length);
    buffer[length - 1] = '\0'; // Ensure null termination
}
