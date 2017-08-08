#include <SPIFlash.h>
#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <SPI.h>
#include <math.h>

#define debug_stop while(1);
#define CMD_NEW_IMG 0xFA
#define CMD_READ_IMG 0xFB
#define CMD_READ_STATUS 0xFC
#define CMD_REPROG_NODE 0xFD

#define CMD_INVALID_CMD 0xFF
#define CMD_CTS 0x01

#define sprint(x) Serial.print(x)
#define sprintln(x) Serial.println(x)

#define SUCCESS 1
#define FAILURE 0

#define ADDR_IMG_START 0x0A
#define MAX_BYTES_PACKET 54


#define MYADDR 2

RH_RF69 driver;
RHReliableDatagram manager(driver, MYADDR);

typedef struct command_mapping
{
  uint8_t command;
  uint8_t (*fp)();
} command_mapping;

typedef struct packet_update
{
  uint8_t type = 0x11;
  uint8_t size;
  uint8_t checksum;
  uint16_t sequence_number;
  uint8_t data[MAX_BYTES_PACKET];
} packet_update;

typedef struct packet_update_request
{
  uint8_t type = 0x22;
  uint16_t number_packets;
  uint16_t size;
} packet_update_request;

uint8_t receive_new_image(void);
uint8_t read_stored_image(void);
uint8_t read_image_status(void);
uint8_t reprogram_node(void);

command_mapping command_map[] =
{ {CMD_NEW_IMG, &receive_new_image},
  {CMD_READ_IMG, &read_stored_image},
  {CMD_READ_STATUS, &read_image_status},
  {CMD_REPROG_NODE, &reprogram_node}
};

uint8_t command_map_size = sizeof(command_map) / sizeof(command_mapping);

SPIFlash flash(8, 0xEF30);

uint8_t test2[] = "HelloWorldHelloWorldHelloWorldHelloWorldHelloWorldHelloWorl";
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];

void setup() {
  Serial.begin(115200);

  if (flash.initialize())
  {
    //Serial.println("SPI Flash Init OK!");
  }
  else
  {
    Serial.println("SPI Flash Init FAIL! (is chip present?)");
    while (1);
  }
  if (!manager.init())
    Serial.println("init failed");
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)

  driver.setTxPower(18, true);



}

void writeUINT32(uint32_t address, uint32_t data)
{
  flash.writeByte(address++, data >> 24);
  flash.writeByte(address++, data >> 16);
  flash.writeByte(address++, data >> 8);
  flash.writeByte(address, data);
}

uint32_t readUINT32(uint32_t address)
{
  uint32_t res = 0;

  res = (uint32_t)(flash.readByte(address++)) << 24;
  res += (uint32_t)(flash.readByte(address++)) << 16;
  res += (uint32_t)(flash.readByte(address++)) << 8;
  res += (uint32_t)(flash.readByte(address));

  return res;
}


void loop() {
  while (!Serial.available());

  uint8_t command = Serial.read();

  uint8_t command_found = 0;
  for (uint8_t command_index = 0; command_index < command_map_size; command_index++)
  {
    if (command == command_map[command_index].command)
    {
      uint8_t status = command_map[command_index].fp();
      command_found = 1;
      if (status)
      {
        //sprintln("Command Succesful");
      }
      else
      {
        sprintln("Command Failed");
      }
      break;
    }
  }

  if (!command_found)
  {
    sprintln(CMD_INVALID_CMD);
  }

}
uint8_t reprogram_node()
{
  sprintln(CMD_CTS);

  while (!Serial.available());

  uint8_t node_id = Serial.read();

  sprintln(node_id);

  if (valid_image()) {
    sprintln(0x01);
    uint32_t image_size = get_image_size();
    sprintln(image_size);
    packet_update_request request;
    uint32_t packets = (uint32_t)ceil((float)image_size / (float)MAX_BYTES_PACKET);
    request.number_packets = packets;
    sprintln(packets);

    packet_update pupdate;
    uint16_t seq_n = 0;
    uint8_t c = 0;
    pupdate.checksum = 0;
    char buff[10];
    for (uint32_t address = ADDR_IMG_START; address < (image_size + ADDR_IMG_START); address++)
    {
      uint8_t data = flash.readByte(address);
      pupdate.data[c] = data;
      // sprintf(buff, "%02X", pupdate.data[c]);
      //  Serial.print(buff);
      c++;
      pupdate.checksum =  pupdate.checksum ^ data;

      if (c >= MAX_BYTES_PACKET || (((image_size + ADDR_IMG_START) - address) == 1) )
      {

        //Send
        //pupdate =
        Serial.print(seq_n);
        Serial.print(":");
        Serial.print(((image_size + ADDR_IMG_START) - address));
        Serial.print(":");
        Serial.println(c);
        pupdate.sequence_number = seq_n;
        pupdate.size = c;
        Serial.println(sizeof(pupdate));
        if (manager.sendtoWait((uint8_t *)&pupdate, sizeof(pupdate), 10))
          // if (manager.sendtoWait(test2, sizeof(test2), 10))
        {
          delay(10);
        }
        else {
          Serial.println("sendtoWait failed");
          break;
        }

        pupdate.checksum = 0;
        c = 0;

        seq_n++;


      }
    }
  }
  else
  {
    sprintln(0x00);
  }


}

inline uint8_t valid_image()
{
  return flash.readByte(0);
}

inline uint32_t get_image_size()
{
  return readUINT32(1);
}
uint8_t read_image_status()
{
  sprintln(CMD_CTS);

  uint8_t valid_image = flash.readByte(0);
  sprintln(valid_image);

  if (valid_image)
  {
    uint32_t image_size = readUINT32(1);
    sprintln(image_size);
  }
  return SUCCESS;
}

uint8_t read_stored_image()
{
  sprintln(CMD_CTS);

  uint8_t valid_image = flash.readByte(0);

  //Fail if there is no image
  if (!valid_image)
  {
    return FAILURE;
  }

  uint32_t image_size = readUINT32(1);
  sprintln(image_size);
  char buff[20];
  for (uint32_t address = ADDR_IMG_START; address < (image_size + ADDR_IMG_START); address++)
  {
    uint8_t data = flash.readByte(address);

    sprintf(buff, "%02X", data);
    Serial.println(buff);

  }
  return SUCCESS;
}


uint8_t receive_new_image()
{
  //Erase first 32 K
  flash.blockErase32K(0);

  //Sent CTS
  sprintln(CMD_CTS );
  uint32_t address = ADDR_IMG_START;

  while (1)
  {
    while (!Serial.available());

    uint8_t size = Serial.read();

    if (size == 0x00)
    {

      writeUINT32(1, (address - ADDR_IMG_START));
      flash.writeByte(0, 0x01);
      //
      return SUCCESS;
    }
    sprintln(size);

    uint8_t data_buffer[32];//TODO: why 32

    uint8_t checksum = 0;

    for (uint8_t byte_count = 0; byte_count < size; byte_count++)
    {
      while (!Serial.available());
      uint8_t data = Serial.read();
      data_buffer[byte_count] = data;
      checksum ^= data;
      flash.writeByte(address++, data);

    }

    while (!Serial.available());
    uint8_t cs = Serial.read();

    if (cs == checksum)
    {

      sprintln(0xff);
    }
    else
    {
      sprintln(0x00);
      sprintln(checksum);
      break;
    }
  }
  return FAILURE;
}




/*
   New Image Protocol
   Send CMD_NEW_IMG
   receive CMD_NEW_IMG_CTS or CMD_INVALID_CMD
*/


