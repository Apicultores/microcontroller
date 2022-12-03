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

String timestamp;
String file_name;

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
  // Tempo de intervalo entre uma medição e outra, em segundos
  const int COLLECT_DATA_TIME_INTERVAL = 60;
  DateTime now = rtc.now();
  int time_last_check = (now - last_time).totalseconds();
  if (time_last_check > COLLECT_DATA_TIME_INTERVAL) {
    last_time = now;
    timestamp = last_time.timestamp(TIMESTAMP_DATE);
    file_name = fmt::format("./{}.json", timestamp.c_str());
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float s = analogRead(SOUNDPIN);

    if (!isnan(t) && !isnan(h)) {
      writeFile(SD, file_name, "{");
      appendFile(SD, file_name, fmt::format("humidity: %.5f,", h));
      appendFile(SD, file_name, fmt::format("temperature: %.1f,", t));
      appendFile(SD, file_name, fmt::format("sound: %f,", s));
      appendFile(SD, file_name, fmt::format("date: %s,", timestamp));
      appendFile(SD, file_name, "}");
    }
  }

  if (SerialBT.available()) {
    char input = (char)SerialBT.read();
    if (input == 'g') {
      start_time = last_time;
      Serial.println("Lendo arquivos...");
      // Le os ultimos 30 dias, onde cada arquivo contém os dados de um dia
      for (int i = 0, i < 30, i++) {
        readFileBT(SD, start_time.timestamp(TIMESTAMP_DATE).c_str());
        start_time = start_time - TimeSpan(ONE_DAY_IN_SECONDS);
      }
    }
  }
}