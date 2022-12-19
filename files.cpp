#include "files.h"



void listDir(fs::FS &fs, const char *dirname) {
  Serial.printf("Listando diretorio %s...\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Falha ao abrir diretório!");
    if (!root.isDirectory()) {
      Serial.println("O caminho especificado não é um diretório");
    }
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.printf("Diretório: %s", file.name());
    } else {
      Serial.printf("Arquivo: %s", file.name());
      Serial.printf("Tamanho: %s", file.size());
    }
    file = root.openNextFile();
  }
}

void listAllDir(fs::FS &fs, const char *rootDirname, uint8_t levels) {
  Serial.printf("Listando diretorio %s...\n", rootDirname);

  File root = fs.open(rootDirname);
  if (!root) {
    Serial.printf("Falha ao abrir diretório!");
    if (!root.isDirectory()) {
      Serial.printf("O caminho especificado não é um diretório");
    }
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.printf("Diretório: %s", file.name());
      if (levels) {
        listAllDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("Arquivo: ");
      Serial.print(file.name());
      Serial.print("Tamanho: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

// TODO remover método caso não utilizarmos para criar pastas no FS
void createDir(fs::FS &fs, const char *path) {
  Serial.printf("Criando diretório: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Sucesso!");
  } else {
    Serial.println("Erro ao criar diretório!");
  }
}

// TODO remover método caso não utilizarmos para remover pastas no FS
void removeDir(fs::FS &fs, const char *path) {
  Serial.printf("Removendo diretório: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Sucesso!");
  } else {
    Serial.println("Erro ao remover diretório!");
  }
}

void fileToFile(fs::FS &fs, const char *source, const char* dest) {
  File sourceFile = fs.open(source);
  File destFile = fs.open(dest, FILE_APPEND);
  while(sourceFile.available()) {
    char readChar = sourceFile.read();
    destFile.write(readChar);
  }
  sourceFile.close();
  destFile.close();
}

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Lendo arquivo: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Falha ao abrir arquivo! read");
    return;
  }

  Serial.print("Conteúdo do arquivo: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void readFileBT(fs::FS &fs, const char *path, BleSerial* SerialBT) {
  Serial.printf("Lendo arquivo: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Falha ao abrir arquivo! read bt");
    return;
  }

  Serial.print("Enviando conteúdo do arquivo via Bluetooth");
  char buffer[251];
  int counter = 0;
  while (file.available()) {
    char readChar = file.read();
    buffer[counter] = readChar;
    counter++;
    if (readChar == '\n' || counter >= 250) {
      buffer[counter] = '\0';
      SerialBT->print(buffer);
      counter = 0;
    }
  }
  if (counter > 0) {
    buffer[counter] = '\0';
    SerialBT->print(buffer);
  }
  file.close();
}

bool checkFileExists(fs::FS &fs, const char *path) {
  return fs.exists(path);
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Escrevendo no arquivo: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Falha ao abrir arquivo! write");
    return;
  }
  if (file.print(message)) {
    Serial.println("Mensagem escrita com sucesso!");
  } else {
    Serial.println("Falha ao escrever no arquivo!");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Anexando ao arquivo: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Falha ao abrir arquivo! append");
    return;
  }
  if (file.print(message)) {
    Serial.println("Mensagem anexada com sucesso!");
  } else {
    Serial.println("Falha ao anexar mensagem no arquivo!");
  }
  file.close();
}

// TODO implementar caso precisemos utilizar
// void renameFile(fs::FS &fs, const char *path1, const char *path2) {
//   Serial.printf("Renomeando aqruivo  file %s to %s\n", path1, path2);
//   if (fs.rename(path1, path2)) {
//     Serial.println("File renamed");
//   } else {
//     Serial.println("Rename failed");
//   }
// }

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deletando arquivo: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("Sucesso!");
  } else {
    Serial.println("Falha ao deletar arquivo!");
  }
}
