#include "uart_lib.h"
#include "xprintf.h"

__attribute__((used, section(".spifi.text")))
void modem_rx(uint8_t modem_rx_buf[], uint32_t modem_rx_buf_length)
{
	xprintf("Modem: RX[%d] %s\n", modem_rx_buf_length, modem_rx_buf);
}
