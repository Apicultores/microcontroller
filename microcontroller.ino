#include "BluetoothSerial.h"
#include "DHT.h"
#include "RTClib.h"
#include "SD.h"
#include "SPI.h"
#include <HTTPUpdateServer.h>
#include <Wire.h>

#include "files.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth não está habilitado!
#endif

#define SOUNDPIN 34
#define DHTPIN 33
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;
BluetoothSerial SerialBT;

// TODO confirmar com o Finger/internet se os nomes das variáveis fazem sentido
const int SERIAL_DATA_FLOW = 115200;
const String BLUETOOTH_NAME = "finger_teste";
const uint8_t SD_CARD_PIN = 5;

void setup() {
  Serial.begin(SERIAL_DATA_FLOW);
  SerialBT.begin(BLUETOOTH_NAME);

  /* DHT */
  dht.begin();
  pinMode(SOUNDPIN, INPUT);
  pinMode(32, OUTPUT);
  digitalWrite(32, 1);

  /* RTC */
  if (!rtc.begin()) {
    Serial.println("Relógio (RTC) não conctado");
  } else if (rtc.lostPower()) {
    Serial.println("Ajustando relógio com horário do PC...");
    rtc.adjust(DateTime(__DATE__, __TIME__));
    Serial.println("Data ajustada para: %s", rtc.now().timestamp());
  } else {
    Serial.println("Data atual: %s", rtc.now().timestamp());
  }

  /* SD */
  if (!SD.begin(SD_CARD_PIN)) {
    Serial.println("Falha ao inicializar módulo SD");
    return;
  }

  if (CARD_NONE == SD.cardType()) {
    Serial.println("Cartão SD não conectado");
    return;
  }

  Serial.printf("Capacidade do cartão SD: %lluMB\n",
                SD.cardSize() / (1024 * 1024));
}

void loop() {
  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available()) {
    char input = (char)SerialBT.read();
    Serial.write(input);
    if (input == 'g') {
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      float s = analogRead(SOUNDPIN);
      // testa se retorno é valido, caso contrário algo está errado.
      if (isnan(t) || isnan(h)) {
        SerialBT.println("Failed to read from DHT");
      } else {
        SerialBT.println("Umidade: %.5f%", h);
        SerialBT.println("Temperatura: %.1f*C", t);
        SerialBT.println("Som: %f", s);
        SerialBT.println("Data: %s", rtc.now().timestamp());
      }
    } else if (input == 'r') {
      readFileBT(SD, "/hello.txt");
      SerialBT.println("");
    } else if (input == 'w') {
      writeFile(SD, "/hello.txt", "Hello ");
      appendFile(SD, "/hello.txt", rtc.now().timestamp().c_str());
    }
  }
}