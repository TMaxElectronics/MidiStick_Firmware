/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.h

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

#include <stdio.h>
#include <xc.h>

void UART_init(int baudRate1, int baudRate2);
void UART_sendString(char *data, unsigned newLine);
void UART_sendInt(unsigned long data, unsigned newLine);
void UART_sendBin(unsigned long long data, char length, unsigned newLine);
void UART_sendChar(char data);
void UART_sendHex(uint32_t data, unsigned newLine);
void UART_clearOERR();
