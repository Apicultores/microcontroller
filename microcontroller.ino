#include "DHT.h"
#include "RTClib.h"
#include "SD.h"
#include "SPI.h"
#include <BleSerial.h>
#include <HTTPUpdateServer.h>
#include <Wire.h>
#include "files.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth não está habilitado!
#endif

/* Define macros */
#define SOUNDPIN 34
#define DHTPIN 33
#define DHT2PIN 27
#define DHTTYPE DHT11
#define ONE_DAY_IN_SECONDS 86400
#define DAYS_RETURNED 7
#define MEASURE_INTERVAL 5*60000

/* Instancia modulos */
DHT dht(DHTPIN, DHTTYPE);
DHT dht2(DHT2PIN, DHTTYPE);
RTC_DS3231 rtc;
BleSerial SerialBT;
DateTime last_time;
String timestamp;
char file_name[100];
const int SERIAL_DATA_FLOW = 115200;
const String BLUETOOTH_NAME = "colmeia_01";
const uint8_t SD_CARD_PIN = 5;

/* Classe responsável por tratar as informações de som */
class SoundCatch {
  public:

  int max;
  int min;
  time_t last_time;
  double sum;
  int count;
  const int time_interval = 100;

  SoundCatch() {
    this->max = 2048;
    this->min = 2048;
    this->sum = 0;
    this->count = 0;
    this->last_time = millis();
  }

  void iter() {
    int val = analogRead(SOUNDPIN);
    if (val > this->max) this->max = val;
    if (val < this->min) this->min = val;
    time_t current_time = millis();
    if ((time_t)(current_time - this->last_time) >= time_interval) {
      this->last_time = current_time;
      this->count  = 1;
      this->sum  = this->max - this->min;
      this->max = 2048;
      this->min = 2048;
    }
  }
  double get_average() {
    return sum/count;
  }

  void clear_count(){
    this->sum = 0;
    this->count = 0;
  }
};

/* Classe responsável por tratar as informações de timestamp */
class TimeKeeper {
  public:

  time_t last_time;
  TimeKeeper() {
    last_time = millis();
  }

  bool time_passed() {
    time_t current_time = millis();
    if ((time_t)(current_time - last_time) >= MEASURE_INTERVAL) {
      last_time = current_time;
      return true;
    }
    return false;
  }
};

DateTime now() {
  DateTime current = rtc.now();
  while (current.year() < 2022 || current.year() > 2100) {
    current = rtc.now();
  }
  return current;
}

SoundCatch* sound_obj = new SoundCatch();
TimeKeeper* time_keeper = new TimeKeeper();

void setup() {
  Serial.begin(SERIAL_DATA_FLOW);
  SerialBT.begin(BLUETOOTH_NAME.c_str());

  /* DHT */
  dht.begin();
  dht2.begin();
  pinMode(SOUNDPIN, INPUT);
  pinMode(32, OUTPUT);
  digitalWrite(32, 1);

  /* RTC */
  if (!rtc.begin()) {
    Serial.println("Relógio (RTC) não conctado");
  } else if (rtc.lostPower()) {
    Serial.println("Ajustando relógio com horário do PC...");
    rtc.adjust(DateTime(__DATE__, __TIME__));
    Serial.printf("Data ajustada para: %s\n", now().timestamp());
  } else {
    Serial.printf("Data atual: %s\n", now().timestamp());
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

  Serial.printf("Capacidade do cartão SD: %lluMB\n", SD.cardSize()/(1024*1024));
  writeFile(SD, "/hello_micro.txt", "Hello");
  readFile(SD, "/hello_micro.txt");

  last_time = now();
}

float giveDefaultValueWhenNaN(float val, float default_val = -100) {
  if (isnan(val)) return default_val;
  return val;
}

float convertToDB(double val) {
  return map((long)val, 0, 4096, 20, 80);
}

void loop() {
  // Tempo de intervalo entre uma medição e outra, em segundos
  sound_obj->iter();
  if (time_keeper->time_passed()) {
    DateTime now_time = now();
    String date, timestamp;
    date = now_time.timestamp(DateTime::timestampOpt::TIMESTAMP_DATE);
    timestamp = now_time.timestamp();
    
    /* Afere dados dos modulos */
    sprintf(file_name, "/%s.json", date.c_str());
    float external_humidity = giveDefaultValueWhenNaN(dht.readHumidity());
    float external_temperature = giveDefaultValueWhenNaN(dht.readTemperature());
    float internal_humidity = giveDefaultValueWhenNaN(dht2.readHumidity());
    float internal_temperature = giveDefaultValueWhenNaN(dht2.readTemperature());
    float sound = convertToDB(sound_obj->get_average());
    sound_obj->clear_count();

    char timestamp_str[50];
    sprintf(timestamp_str, "%s", timestamp.c_str());
    timestamp_str[10] = ' ';

    /* Escreve os dados coletados em formato JSON */
    char to_write[100];
    sprintf(to_write,"{\"ti\":%.1f,\"te\":%.1f,\"ui\":%.1f,\"ue\":%.1f,\"s\":%.1f,\"ts\":\"%s\"}",
        internal_temperature, external_temperature, internal_humidity, external_humidity, sound, timestamp_str);

    if(checkFileExists(SD, file_name)){
      appendFile(SD, file_name, ",\n");
      appendFile(SD, file_name, to_write);
    } else {
      writeFile(SD, file_name, to_write);
    }
  }

  /*Envia dados via Bluetooth */
  if (SerialBT.available()) {
    char input = (char)SerialBT.read();

    if (input == 'd') {
      DateTime now_time = now();
      String date, timestamp;
      date = now_time.timestamp(DateTime::timestampOpt::TIMESTAMP_DATE);
      timestamp = now_time.timestamp();

      float external_humidity = giveDefaultValueWhenNaN(dht.readHumidity());
      float external_temperature = giveDefaultValueWhenNaN(dht.readTemperature());
      float internal_humidity = giveDefaultValueWhenNaN(dht2.readHumidity());
      float internal_temperature = giveDefaultValueWhenNaN(dht2.readTemperature());
      float sound = convertToDB(sound_obj->get_average());

      char timestamp_str[50];
      sprintf(timestamp_str, "%s", timestamp.c_str());
      timestamp_str[10] = ' ';

      char to_write[100];
      sprintf(to_write,"{\"ti\":%.1f,\"te\":%.1f,\"ui\":%.1f,\"ue\":%.1f,\"s\":%.1f,\"ts\":\"%s\"}",
          internal_temperature, external_temperature, internal_humidity, external_humidity, sound, timestamp_str);

      Serial.println(to_write);
    } else if (input == 'g') {
      DateTime start_time = now() - TimeSpan((DAYS_RETURNED-1)*ONE_DAY_IN_SECONDS);
      Serial.println("Lendo arquivos...");
      SerialBT.print("{\"data\": [");
      // Le os ultimos arquivos, onde cada arquivo contém os dados de um dia
      bool has_written = false;
      for (int i = 0; i < DAYS_RETURNED; i++) {
        sprintf(file_name, "/%s.json", start_time.timestamp(DateTime::timestampOpt::TIMESTAMP_DATE).c_str());
        start_time = start_time + TimeSpan(ONE_DAY_IN_SECONDS);
        if (!checkFileExists(SD, file_name)) continue;
        if (has_written) {
          SerialBT.print(",\n");
        }
        readFileBT(SD, file_name, &SerialBT);        
        has_written = true;
      }

      SerialBT.print("\n]}\n");
      // Envia o caractere que indica o final de um arquivo de dados
      SerialBT.print("@\n");
    }
  }
}