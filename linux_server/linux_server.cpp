#include <sys/socket.h> //для сокетов
#include <netinet/in.h> //для связи с интернетом (тут определены заголовки IPv4 и IPv6)
#include <unistd.h> //для связи с POSIX операционными системами
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <iostream>
#include <ifaddrs.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 64
#define BUFFER_SIZE 1024

int connections[MAX_CLIENTS]; //список подключенных пользоваталей
int connect_soc; //отдельный подключенный пользователь
int listen_soc; //слушающий сокет сервера - он принимает и закрывает подключения клиентов метод accept()
int client_count = 0;

int disconnect_client(int client_id){

    if (client_id < 0 || client_id >= MAX_CLIENTS) {
        std::cerr << "Invalid client ID" << std::endl;
        return -1;
    }

    char message[] = "bye! im closing connection!";
    send(connections[client_id], message, sizeof(message), 0);
    close(connections[client_id]);
    connections[client_id] = -1;
    std::cout<<"client " << client_id << " disconnected" << std::endl;
    client_count--;

    return 0;
}

int main(){
    setvbuf(stdout, NULL, _IOLBF, 0);
    //std::cout<<"tst" << std::endl;
    setlocale(LC_ALL, "russian");
    
    if((listen_soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))<0){
        perror("failed to create main socket!");
        exit(EXIT_FAILURE);
    } //создадаем серверный сокет с параметрами 1)IPv4, 2)Потоковый сокет(не датаграмный) 3)протокол TCP 
    //это мы настроили сам сокет!! теперь нужно дать ему информацию о хосте! - это функция bind()

    sockaddr_in host; //сюда получаем параметры нашего сервера

    host.sin_family = AF_INET; //семейство адресов, которые будем принимать (IPv4)
    host.sin_addr.s_addr = INADDR_ANY; //говорим нашему сокету слушать все айпи адреса
    host.sin_port = htons(1234); //здесь задали порт

    if(bind(listen_soc, (struct sockaddr*)&host, sizeof(host)) < 0){
        perror("failed to bind main socket!");
        exit(EXIT_FAILURE);
    }
    //мы закрепили наш сокет на порте 1234 и сказали ему слушать все айпи адреса и работать с IPv4
    //теперь нам надо запустить его - перевести в состояние listen - он будет ждать, когда клиентский сокет постучится к нам на сервер

    if(listen(listen_soc, MAX_CLIENTS) < 0){
        perror("failed to enable listen socket!");
        exit(EXIT_FAILURE);
    }

    printf("\nServer started on host 1234\n");
    //std::cout << "server started on host 1234" <<std::endl;

    struct ifaddrs *addrs;
    getifaddrs(&addrs);  // Получить список всех интерфейсов

    for (struct ifaddrs *ifa = addrs; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {  // Только IPv4
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
            printf("Interface: %s, IP: %s\n", ifa->ifa_name, ip);
        }
    }

    freeifaddrs(addrs);

    //мы запустили наш сокет! теперь сервер работает и нам нужно вызвать accept в бесконечном цикле, чтобы принимать подключения

    char connect_message[] = "welcome to server";

    setvbuf(stdout, NULL, _IOLBF, 0);
    
    while(true){
        if((connect_soc = accept(listen_soc, nullptr, nullptr)) >= 0){
            connections[client_count] = connect_soc;
            std::cout<< "\nclient " << client_count << " connected"<< std::endl;

            send(connect_soc, connect_message, sizeof(connect_message), 0);
            client_count++;
            
            disconnect_client(client_count-1);
        }
        else {
            perror("Accept failed");
            continue;
        }
    }
    close(listen_soc);
    return 0;
}