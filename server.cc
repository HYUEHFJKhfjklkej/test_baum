#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>

// Хранит информацию о последовательности
struct Sequence {
  uint64_t current;
  uint64_t step;
};

// Общее хранилище настроек клиентов
std::map<int, std::map<std::string, Sequence>> clientSettings;
std::mutex clientSettingsMutex;

// Обрабатывает соединение с клиентом
void handleConnection(int clientSocket) {
  char buffer[1024] = {0};
  std::string command;

  while (true) {
    ssize_t valread = read(clientSocket, buffer, 1024);
    if (valread <= 0) {
      break;
    }

    std::istringstream iss(buffer);
    iss >> command;

    // Определить тип команды
    if (command == "seq1" || command == "seq2" || command == "seq3") {
      uint64_t start, step;
      iss >> start >> step;

      Sequence sequence = {start, step};

      std::lock_guard<std::mutex> guard(clientSettingsMutex);
      clientSettings[clientSocket][command] = sequence;

      // Ответить клиенту
      std::string response = "Установлено: " + command + "\n";
      send(clientSocket, response.c_str(), response.size(), 0);

    } else if (command == "export") {
      std::lock_guard<std::mutex> guard(clientSettingsMutex);
      auto& settings = clientSettings[clientSocket];

      std::string response;
      for (auto& [seqName, seq] : settings) {
        response += std::to_string(seq.current) + "\t";
        seq.current += seq.step;
      }
      response += "\n";

      send(clientSocket, response.c_str(), response.size(), 0);
    }

    std::fill(std::begin(buffer), std::end(buffer), 0);
  }

  close(clientSocket);
  std::cout << "Соединение закрыто.\n";
}

int main() {
  int server_fd;
  struct sockaddr_in address;
  int opt = 1;
  socklen_t addrlen = sizeof(address);  // замена int на socklen_t

  // Создание сокета
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    std::cerr << "Ошибка создания сокета\n";
    exit(EXIT_FAILURE);
  }

  // Прикрепление сокета к порту 8080
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    std::cerr << "Ошибка прикрепления сокета\n";
    exit(EXIT_FAILURE);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(8080);

  if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
    std::cerr << "Ошибка привязки\n";
    exit(EXIT_FAILURE);
  }
  if (listen(server_fd, 3) < 0) {
    std::cerr << "Ошибка прослушивания\n";
    exit(EXIT_FAILURE);
  }

  while (true) {
    std::cout << "Ожидание соединения...\n";

    int newSocket;
    if ((newSocket = accept(server_fd, (struct sockaddr*)&address,
                            &addrlen)) < 0) {  // исправленный вызов accept
      std::cerr << "Ошибка приема\n";
      exit(EXIT_FAILURE);
    }

    std::cout << "Соединение принято.\n";

    std::thread connectionThread(handleConnection, newSocket);
    connectionThread.detach();
  }

  return 0;
}
