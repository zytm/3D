#include <DHT.h>
#include <U8g2lib.h>
#include <U8x8lib.h>

#define DHTTYPE    DHT22

char temperature [5];
char humidity [5];
char temperature1 [5];
char humidity1 [5];
const char DEGREE_SYMBOL[] = { 0xB0, '\0' };

DHT dht(6, DHTTYPE);
DHT dht1(7, DHTTYPE);

U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, /* CS=*/ 10, /* reset=*/ 8);

void setup() {

  dht.begin();
  dht1.begin();
  Serial.begin(9600);
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_6x10_mf);
  u8g2.setColorIndex(1);
}

void loop() {
  u8g2.firstPage();
  do {
    draw();
  } while ( u8g2.nextPage() );
}

void draw() {

  readTemperature();
  readHumidity();
  readTemperature1();
  readHumidity1();

  u8g2.drawFrame(0, 0, 64, 14);
  u8g2.drawFrame(63, 0, 65, 14);
  u8g2.drawFrame(0, 13, 128, 14);
  u8g2.drawFrame(0, 26, 64, 13);
  u8g2.drawFrame(63, 26, 65, 13);
  u8g2.drawFrame(0, 38, 128, 14);
  u8g2.drawFrame(0, 51, 64, 13);
  u8g2.drawFrame(63, 51, 65, 13);
    
  u8g2.drawStr( 30, 23, "Temperature");
  u8g2.drawStr( 14, 10, "Printer");
  u8g2.drawStr( 70, 10, "Storage");
  u8g2.drawStr( 16, 36, temperature);
  u8g2.drawUTF8(40, 36, DEGREE_SYMBOL);
  u8g2.drawUTF8(45, 36, "C");
  u8g2.drawStr(37, 48, "Humidity");
  u8g2.drawStr( 16, 61, humidity);
  u8g2.drawStr(48, 61, "%");

  u8g2.drawStr( 80, 36, temperature1);
  u8g2.drawUTF8(105, 36, DEGREE_SYMBOL);
  u8g2.drawUTF8(110, 36, "C");
  u8g2.drawStr( 80, 61, humidity1);
  u8g2.drawStr(112, 61, "%");
}

void readTemperature()
{
  float t = dht.readTemperature();
  dtostrf(t, 3, 1, temperature);
}

void readHumidity()
{
  float h = dht.readHumidity();
  dtostrf(h, 3, 1, humidity);
}


//

void readTemperature1()
{
  float t1 = dht1.readTemperature();
  dtostrf(t1, 3, 1, temperature1);
}

void readHumidity1()
{
  float h1 = dht1.readHumidity();
  dtostrf(h1, 3, 1, humidity1);
}
