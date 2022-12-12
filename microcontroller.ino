#include <BleSerial.h>
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
#define ONE_DAY_IN_SECONDS 86400
#define THIRTY_DAYS_IN_SECONDS 30*ONE_DAY_IN_SECONDS

DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;
BleSerial SerialBT;
DateTime last_time;

String timestamp;
char file_name[100];

const int SERIAL_DATA_FLOW = 115200;
const String BLUETOOTH_NAME = "finger_teste";
const uint8_t SD_CARD_PIN = 5;

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

class TimeKeeper {
  public:
  time_t last_time;
  TimeKeeper() {
    last_time = millis();
  }
  bool minute_passed() {
    time_t current_time = millis();
    if ((time_t)(current_time - last_time) >= 60000) { // 1 s pra teste, trocar pra 60000
      last_time = current_time;
      return true;
    }
    return false;
  }
};

class BluetoothWriterDelegate {
  public:
  File currentFile;
  bool hasfile;

  BluetoothWriterDelegate() {
    hasfile = false;
  }
  int setFile(String filename) {
    if (writing()) { return -1; }
    if (!checkFileExists(SD, filename.c_str())) { return -1; }
    currentFile = SD.open(filename.c_str());
  }
  bool writing() {
    if (hasfile && currentFile.available()) return true;
    if (hasfile) currentFile.close();
    hasfile = false;
    return false;
  }
  void iter() {
    if (writing()) {
      while ( currentFile.available() && SerialBT.availableBufferSpace() > 0) {
        char readChar = currentFile.read();
        SerialBT.write(readChar);
      }
    }
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

  Serial.printf("Capacidade do cartão SD: %lluMB\n",
                SD.cardSize() / (1024 * 1024));

  writeFile(SD, "/hello_micro.txt", "Hello");
  readFile(SD, "/hello_micro.txt");

  last_time = now();
}

void loop() {
  // Tempo de intervalo entre uma medição e outra, em segundos
  sound_obj->iter();
  // const int COLLECT_DATA_TIME_INTERVAL = 60;
  // DateTime now = rtc.now();
  // int time_last_check = (now - last_time).totalseconds();
  if (time_keeper->minute_passed()) {
    // last_time = now;
    DateTime now_time = now();
    String date, timestamp;
    date = now_time.timestamp(DateTime::timestampOpt::TIMESTAMP_DATE);
    timestamp = now_time.timestamp();
    
    sprintf(file_name, "/%s.json", date.c_str());
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    float sound = sound_obj->get_average();
    sound_obj->clear_count();

    if (!isnan(temperature) && !isnan(humidity)) {

      char timestamp_str[50];
      sprintf(timestamp_str, "%s", timestamp.c_str());
      timestamp_str[10] = ' ';

      char to_write[100];
      sprintf(to_write,"{\"ti\":%.1f,\"te\":%.1f,\"ui\":%.1f,\"ue\":%.1f,\"s\":%.1f,\"ts\":\"%s\"}\n",
          temperature, temperature, humidity, humidity, sound, timestamp_str);

      if(checkFileExists(SD, file_name)){
        appendFile(SD, file_name, to_write);
      } else {
        writeFile(SD, file_name, to_write);
      }
    }
  }

  if (SerialBT.available()) {
    char input = (char)SerialBT.read();
    if (input == 'g') {
      DateTime start_time = now() - TimeSpan(THIRTY_DAYS_IN_SECONDS) + TimeSpan(ONE_DAY_IN_SECONDS);
      Serial.println("Lendo arquivos...");
      // SerialBT.print("{\"data\": [");
      // Le os ultimos 30 dias, onde cada arquivo contém os dados de um dia
      for (int i = 0; i < 30; i++) {
        sprintf(file_name, "/%s.json", start_time.timestamp(DateTime::timestampOpt::TIMESTAMP_DATE).c_str());
        readFileBT(SD, file_name, &SerialBT);
        readFile(SD, file_name);
        start_time = start_time + TimeSpan(ONE_DAY_IN_SECONDS);
      }
      SerialBT.print("!@##@!\n");
    }
  }
}