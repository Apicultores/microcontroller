#include "files.h"

// TODO quebrar em listDir e listAllDir
void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
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
      Serial.println("Diretório: %s", file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
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

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Lendo arquivo: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Falha ao abrir arquivo!");
    return;
  }

  Serial.print("Conteúdo do arquivo: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void readFileBT(fs::FS &fs, const char *path) {
  Serial.printf("Lendo arquivo: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Falha ao abrir arquivo!");
    return;
  }

  Serial.print("Enviando conteúdo do arquivo via Bluetooth");
  while (file.available()) {
    SerialBT.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Escrevendo no arquivo: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Falha ao abrir arquivo!");
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
    Serial.println("Falha ao abrir arquivo!");
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