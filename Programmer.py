#!/bin/python

import sys
import serial
import os.path
import math
import time

CMD_NEW_IMG = 0xFA
CMD_READ_IMG = 0xFB
CMD_READ_IMG_STAT = 0xFC
CMD_INVALID_CMD = 0xFF
CMD_REPROG_NODE = 0xFD
CMD_NEW_IMG_CTS = 0x01

#sends a firmware image over serial 
def send_image():
        
        #write command byte
	Gateway.write(chr(CMD_NEW_IMG))

	#check response is ctchrs
	resp = Gateway.readline()
	resp = resp.strip()
	print int(resp)

        #if we got the right response, proceed
	if(int(resp) == int(CMD_NEW_IMG_CTS)):
		for HexLine in Hex:
			HexLine = HexLine.strip()
			Header = HexLine[:9]
		        Checksum = HexLine[-2:]
		        HexLine = HexLine[9:-2]
		
                        #If we have reached the end of the file notify the client by transmitting a size of 0
                        #and break
			if(Header == ":00000001"):
				print "EOF"
				Gateway.write(chr(0x00))
				break;
			print Header
			byteValues = bytearray.fromhex(HexLine)
			nbytes = len(byteValues)

			print nbytes

			Gateway.write(chr(nbytes))
		
			print Gateway.readline()
			checksum = 0

			for x in byteValues:
				Gateway.write(chr(x))
				checksum = checksum ^ x
				#print x	
			Gateway.write(chr(checksum))	
	
			valid = Gateway.readline()
			print int(valid)	

                        #If the checksums computed at both ends dont match, stop
			if(int(valid) != int(0xff)):
				cs = Gateway.readline()
				print checksum, cs
				print "Error"
				break;
	else:
		print "Error did not receive expected response to command"

def read_image():
	Gateway.write(chr(CMD_READ_IMG))

	resp = Gateway.readline()
	resp = resp.strip()

	print int(resp)

        size = Gateway.readline()
        size = size.strip()

        size = int(size)

        lc = 0
        line = ""
        count = 0
	while True:
		line= line+Gateway.readline().strip()
                lc = lc+1
                count = count +1
                if lc >= 16:
                    print line
                    lc=0
                    line=""

                if count == size:
                    print line
                    break

	#if(int(resp) == int(CMD_NEW_IMG_CTS)):

def read_image_status():
	Gateway.write(chr(CMD_READ_IMG_STAT))
	Gateway.readline() #swallow cts
	valid = Gateway.readline()
	valid = valid.strip()
	if(int(valid) == int(0x01)):		
		size = Gateway.readline()
		size = size.strip()
		print "Image found, size=",size," bytes"
	else:
		print "No Image"

def reprogram_node(nodeID):
	Gateway.write(chr(CMD_REPROG_NODE))
	cts = Gateway.readline()
        cts = cts.strip()

        if(int(cts) == int(CMD_NEW_IMG_CTS)):
		print "Reprogramming request accepted"
		Gateway.write(chr(nodeID))
		print Gateway.readline()
		while True:
			print Gateway.readline()


#Do something better for parsing arguments, alright for now
if len(sys.argv) < 4:
        print 'Usage \"python ProgramNode <SERIALDEVICE> <NODEADDR> <HEXFILE.HEX>\"'
        quit()

if not os.path.isfile(sys.argv[3]):
        print "Specified hex file not found"
        quit()
node_addr = 0
try:
    node_addr = int(sys.argv[2])
except ValueError:
    print("Couldn't convert node address into a number")
    quit()

Hex = open(sys.argv[3], 'r')
LineCount = sum(1 for line in open(sys.argv[3]))

PacketCount = int(math.ceil(LineCount / 3))
print "DBG::lines in file=", LineCount, ", resulting number of packets=",PacketCount
Gateway = serial.Serial(sys.argv[1], 115200)

#Give node a change to initialise
time.sleep(2) 

for option in sys.argv:
    if option == "-status":
        read_image_status()
    if option == "-read":
        read_image()
    if option == "-write":
        send_image()
    if option == "-reprog":

        reprogram_node(10)
#send_image()
#read_image_status()
#read_image()
#reprogram_node(10)

print "Finished"
print Gateway.readline()
Gateway.close()


