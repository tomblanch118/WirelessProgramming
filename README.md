Wireless Reprogramming Library

Aim to provide the ability to wireless reprogram microcontrollers via an RFM69(HW) module. Tested on the Moteino lineof boards and on some custom boards using the Atmega 1284p. 

This project is aimed more at field repogramming of nodes in a sensor network and as such the aim is to be able to flash nodes without the aid of a laptop (which is heavy and not very weatherproof). The procedure is intended to go something like:

1)The firmware image is loaded onto the external flash chip of the "reprogrammer" board via a serial connection. (potentially multiple firmwares could be stored)
1a)The image can be verified either by crc of the whole image or by reading back the image.
2)The reprogrammer board can be disconnected from the computer and taken to the location of the sensor network
3)A sensor node address is specified via the repogrammer and wireless reprogramming is initiated.


