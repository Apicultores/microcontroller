#ifndef FILES_H_
#define FILES_H_

#include "FS.h"
#include "BluetoothSerial.h"

void listDir(fs::FS &fs, const char *dirname);
void listAllDir(fs::FS &fs, const char *dirname, uint8_t levels);
void createDir(fs::FS &fs, const char *path);
void removeDir(fs::FS &fs, const char *path);
void readFile(fs::FS &fs, const char *path);
void readFileBT(fs::FS &fs, const char *path, BluetoothSerial SerialBT);
bool checkFileExists(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
// void renameFile(fs::FS &fs, const char *path1, const char *path2);
void deleteFile(fs::FS &fs, const char *path);

#endif // FILES_H_
