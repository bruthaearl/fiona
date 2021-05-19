
# ezo_uart_lib_test

Test code for the Atlas Scientfic UART EZO library

## Installing the Library

 1. Download the library zip file from [here](https://github.com/Atlas-Scientific/Ezo_uart_lib/archive/refs/heads/master.zip)
 2. Add the library to the Arduino IDE by going to Sketch > Include Library > Add Zip Library and then select the downloaded zip file

## ezo_uart_lib_test.ino
I modified this code from the example code in the EZO UART library repo. The example code loops through three different sensors and this test code only uses one.

The code waits for your input before anything happens. You interact with the sensor similar to the last basic UART code but this time you need to include the sensor number when sending commands through the serial monitor. Because you only have one extra serial port, the sensor number will always be 0 (remember zero-indexing?).

To get the status of status of the sensor, type `0:Status<cr>` into the serial monitor where `<cr>` means hit the enter key. To get a single reading from the sensor, type in `0:R<cr>`. See the pattern? If you had an additional serial port, you could get the status of a second sensor by typing in `1:Status<cr>` but I guess that's irrelevant. All of the UART commands are listed in the [datasheet](https://atlas-scientific.com/files/DO_EZO_Datasheet.pdf), starting on page 20.

