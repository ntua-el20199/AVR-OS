void usart_init(unsigned int ubrr);
void usart_transmit(uint8_t data);
void usart_send_string(const char *s);
uint8_t usart_receive();
char* uart_receive_command();
