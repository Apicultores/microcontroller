#include "BluetoothSerial.h"
#include "DHT.h"
#include "RTClib.h"
#include "SD.h"
#include "SPI.h"
#include <HTTPUpdateServer.h>
#include <Wire.h>
#include <fmt/core.h>

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
DateTime last_time;

String timestamp_string;
String file_name;

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
  last_time = rtc.now();
}

void loop() {
  DateTime now = rtc.now();
  int time_last_check = (now - last_time).totalseconds();
  if (time_last_check > 300) {
    last_time = now;
    timestamp_string = last_time.timestamp().c_str();
    file_name = fmt::format("./{}.json", timestamp_string);
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float s = analogRead(SOUNDPIN);
  
    if (!isnan(t) && !isnan(h)){
      writeFile(SD, file_name, "{");
      appendFile(SD, file_name, fmt::format("humidity: %.5f,", h));
      appendFile(SD, file_name, fmt::format("temperature: %.1f,", t));
      appendFile(SD, file_name, fmt::format("sound: %f,", s));
      appendFile(SD, file_name, fmt::format("date: %s,", timestamp_string));
      appendFile(SD, file_name, "}");
    }
  }

  if (SerialBT.available()) {
    char input = (char)SerialBT.read();
    if (input == 'r') {
      readFileBT(SD, "/hello.txt");
      SerialBT.println("");
    }
  }
}