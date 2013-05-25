
//#include <avr/io.h>
//#include <avr/pgmspace.h>
//#include <avr/interrupt.h>
//#include <string.h>
//#include "drivers/uart.h"
//#include "drivers/xitoa.h"
#include "drivers/uart.h"
#include "drivers/diskio.h"
#include "drivers/vs1063.h"
#include "fatfs/ff.h"

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>


//FUSES = {0xAF, 0xC3, 0xFF};           
/* ATmega64 fuses: Low, High, Extended.
This is the fuse settings for this project. The fuse bits will be included
in the output hex file with program code. However some old flash programmers
cannot load the fuse bits from hex file. If it is the case, remove this line
and use these values to program the fuse bits. */

DWORD get_fattime(void)
{
    return 0;
}

int main(void)
{
    FATFS FileSystemObject;
    DSTATUS driveStatus;
    FIL logFile;
    unsigned int bytesWritten;

    uart_init(UART_BAUD_SELECT(9600,F_CPU));
    //vs_setup();

    DDRD |= 16; // cs is output

    sei();
    uart_puts_P("hello\r\n");

    if (f_mount(0, &FileSystemObject) != FR_OK) {
        //flag error
        uart_puts_P("err: f_mount\r\n");
    }

    driveStatus = disk_initialize(0);

    if (driveStatus & STA_NOINIT) {
        uart_puts_P("err: noinit\r\n");
    }
    if (driveStatus & STA_NODISK) {
        uart_puts_P("err: nodisk\r\n");
    }
    if (driveStatus & STA_PROTECT) {
        uart_puts_P("err: protect\r\n");
    }
     
    if (f_open(&logFile, "FOO.TXT", FA_READ | FA_WRITE | FA_OPEN_ALWAYS) !=
        FR_OK) {
        //flag error
        uart_puts_P("err: f_open\r\n");
    } else {
        f_write(&logFile, "New log opened!\n", 16, &bytesWritten);
        f_sync(&logFile);
        f_close(&logFile);
        uart_puts_P("weeee\r\n");
    }

    f_mount(0, 0);
    uart_puts_P("end\r\n");

    //vs_setup();
    
    for(;;)
    {
    }

}
