# AVR-OS
This is an Operating System built for an AVR ATmega328PB Xplained MINI using C. 
Our approach was to make it as lightweight as possible in order to test the capabilities of an os for a microcontroller with limited resources.

The OS was made for the communication between the AVR and the peripherals in NTUAboard-G1 used in the Microprocessors Lab in the School of Electrical and Computer Engineering of National Technical University of Athens.

#### Functionality
User interaction with a Command-Line Interface.
- User login and register using the EEPROM (credentials saved even after poweroff)
- Read and Write operations for PORTs.
- Read values from Potentiometers.
- Read room temperature using a thermometer sensor
