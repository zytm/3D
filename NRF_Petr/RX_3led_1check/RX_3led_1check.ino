/*   Данный скетч делает следующее: передатчик (TX) отправляет массив
     данных, который генерируется согласно показаниям с кнопки и с
     двух потенциомтеров. Приёмник (RX) получает массив, и записывает
     данные на реле, сервомашинку и генерирует ШИМ сигнал на транзистор.
    by AlexGyver 2016
*/

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <Servo.h>

RF24 radio(9, 10); // "создать" модуль на пинах 9 и 10 Для Уно


byte recieved_data[3]; // массив принятых данных
byte led1 = 2; // led 1
byte led2 = 3; // led 2
byte led3 = 4; // led 3
byte led4 = 5; // проверка

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб

void setup() {
  Serial.begin(9600); //открываем порт для связи с ПК

  pinMode(led1, OUTPUT); // настроить led реле как выход
  pinMode(led2, OUTPUT); // настроить led реле как выход
  pinMode(led3, OUTPUT); // настроить led реле как выход
  pinMode(led4, OUTPUT); // настроить led реле как выход

  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах

  radio.openReadingPipe(1, address[0]);     //хотим слушать трубу 0
  radio.setChannel(0x60);  //выбираем канал (в котором нет шумов!)

  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp(); //начать работу
  radio.startListening();  //начинаем слушать эфир, мы приёмный модуль
}

void loop() {
  byte pipeNo;
  while ( radio.available(&pipeNo)) {  // слушаем эфир со всех труб
    radio.read( &recieved_data, sizeof(recieved_data) );         // чиатем входящий сигнал
    digitalWrite(led1, recieved_data[0]); // подать на реле сигнал с 0 места массива
    digitalWrite(led2, recieved_data[1]); // подать на реле сигнал с 0 места массива
    digitalWrite(led3, recieved_data[2]); // подать на реле сигнал с 0 места массива
    if(digitalRead(2)|| digitalRead(3) || digitalRead(4) == HIGH)
      {
        digitalWrite(5, HIGH);
        delay(2000);
        digitalWrite(5, LOW);
      }
  }
}
