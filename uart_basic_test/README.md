# DO_UART_test.ino
I ripped this code directly from https://atlas-scientific.com/files/Arduino-Mega-DO-Sample-Code.pdf and made some modifications.

The first serial port - the port used by the serial monitor - is simply called "Serial". You can transmit to the serial monitor on the first port by using `Serial.print("Hello!")` . Receiving data is a little bit trickier. Look at how its done in the `serialEvent()` function block.

The second serial port - the port used to communicate with the carrier board - is called "Serial1". You can transmit to the carrier board by using `Serial1.print("Hello!")` (note the subtle difference). 

This code takes whatever you send from the serial monitor and sends it to the carrier board and vice versa. Check out the UART reference on page 20 here: https://atlas-scientific.com/files/DO_EZO_Datasheet.pdf

If you type "Status" into the serial monitor and hit enter (aka carriage return aka `<cr>`), hopefully you'll get a response that looks like what you see on page 35. Checking status is good way of making sure everything is working ok.

If "Status" works, try some of the other commands listed on page 20.
