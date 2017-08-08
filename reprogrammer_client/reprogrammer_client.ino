#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <SPI.h>
#include <SPIFlash.h>

#include <avr/wdt.h>
#define Reset_AVR() wdt_enable(WDTO_1S); while(1) {}
#define MYADDR 10

// Singleton instance of the radio driver

//RH_RF69 driver(4, 2); // For MoteinoMEGA
RH_RF69 driver;
RHReliableDatagram manager(driver, MYADDR);

#define ADDR_IMG_START 0x0A
#define MAX_BYTES_PACKET 54

typedef struct packet_update
{
  uint8_t type = 0x11;
  uint8_t size;
  uint8_t checksum;
  uint16_t sequence_number;
  uint8_t data[MAX_BYTES_PACKET];
} packet_update;

SPIFlash flash(8, 0xEF30);

void setup()
{
  Serial.begin(115200);

  Serial.println("V0.1");

  if (flash.initialize())
  {
    //Serial.println("SPI Flash Init OK!");
  }
  else
  {
    Serial.println("SPI Flash Init FAIL! (is chip present?)");
    while (1);
  }

  while (!Serial)
    ;
  if (!manager.init())
    Serial.println("init failed");
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  driver.setTxPower(18, true);
  flash.blockErase32K(0);
}

uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
uint8_t c = 0;
char buff[10];
uint32_t address = 0;
void loop()
{
  if (manager.available())
  {
    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAck(buf, &len, &from))
    {
      if (buf[0] == 0x11)
      {
        packet_update * pupdt = (packet_update *)&buf;
        Serial.println(pupdt->size);
        for (uint8_t i = 0; i < pupdt->size; i++)
        {

          flash.writeByte(ADDR_IMG_START + address, pupdt->data[i]);

          address++;
        }

        if (pupdt->size < MAX_BYTES_PACKET)
        {
          Serial.print("Done\nfile size=");
          Serial.println((address));
          uint16_t temp_size = (uint16_t)(address);
          flash.writeByte(0, 'F');
          flash.writeByte(1, 'L');
          flash.writeByte(2, 'X');
          flash.writeByte(3, 'I');
          flash.writeByte(4, 'M');
          flash.writeByte(5, 'G');
          flash.writeByte(6, ':');
          flash.writeByte(9, ':');
          flash.writeByte(7, temp_size >> 8);
          flash.writeByte(8, temp_size);
          Reset_AVR()
        }
      }
    }
  }
}

void writeUINT32(uint32_t address, uint32_t data)
{
  flash.writeByte(address++, data >> 24);
  flash.writeByte(address++, data >> 16);
  flash.writeByte(address++, data >> 8);
  flash.writeByte(address, data);
}
