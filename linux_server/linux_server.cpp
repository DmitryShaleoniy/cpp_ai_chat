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
#include <pqxx/pqxx>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
//#include <./chat.cpp>
#include <ctime>

#define MAX_CLIENTS 64
#define BUFFER_SIZE 1024

std::thread send_thread;
std::thread rec_thread;

std::mutex image_mutex;
std::mutex client_count_mutex;

pthread_t thread;

int connections[MAX_CLIENTS]; //список подключенных пользоваталей
int connect_soc; //отдельный подключенный пользователь
int listen_soc; //слушающий сокет сервера - он принимает и закрывает подключения клиентов метод accept()
int client_count = 0;

std::string token;

std::string auth_key = "auth_key_here";

void GetToken(){
    std::string query = "curl -L -X POST \'https://ngw.devices.sberbank.ru:9443/api/v2/oauth\' \
-H \'Content-Type: application/x-www-form-urlencoded\' \
-H \'Accept: application/json\' \
-H \'RqUID: 30be8535-de77-47fb-a518-1b27696c7673\' \
-H \'Authorization: Basic " + auth_key + "\' \
--data-urlencode \'scope=GIGACHAT_API_PERS' -k";
    char buffer[2048];
    token.clear();
    FILE *fp = popen(query.c_str(), "r");
    while (fgets(buffer, 2048, fp)){
        token += buffer;
    }
    fclose(fp);
    int num_start = token.find("access_token") + 15;
    int num_end = token.find("expires_at") - 3;
    token = token.substr(num_start, num_end - num_start);
}

void GetImage(std::string prompt, std::string name){
    std::string ref; //запрашиваем картинку
    std::string query = "curl -L -X POST \'https://gigachat.devices.sberbank.ru/api/v1/chat/completions\' \
-H \'Content-Type: application/json\' \
-H \'Accept: application/json\' \
-H \'Authorization: Bearer "+ token +"\' \
--data-raw \'{\
  \"model\": \"GigaChat\", \
  \"messages\": [\
    {\
      \"role\": \"system\",\
      \"content\": \"Ты — Василий Кандинский\"\
    },\
    {\
      \"role\": \"user\",\
      \"content\": \"Draw me a "+ prompt +"\"\
    }\
   ],\
   \"function_call\": \"auto\"\
}\' -k";
    
    FILE *fp = popen(query.c_str(), "r");
    char buffer[1024];
    while(fgets(buffer, 1024, fp)){
        std::cout<<buffer<<std::endl; //получили ответ в буфер
    }
    fclose(fp); 
    //выделим из буфера ссылку на картинку
    char cline[64];

    char * start_ptr;
    char * end_ptr;
    start_ptr = strstr(buffer, "img src");
    end_ptr = strstr(buffer, "fuse=");
    int start_num = start_ptr - buffer +  10;
    int size = (end_ptr - buffer - 3) - start_num;

    strncpy(cline, start_ptr + 10, size);
    cline[size] = '\0';
    ref.clear();
    ref += cline;
    std::cout<<std::endl;
    std::cout<< ref <<std::endl;

    //запишем картинку в name.img
    query.clear();
    query="curl -L -X GET \'https://gigachat.devices.sberbank.ru/api/v1/files/"+ref+"/content\' -o \"" +name+".jpg\" \
-H \'Accept: application/jpg\' \
-H \'Authorization: Bearer " + token +"\' -k";
    fp = popen(query.c_str(), "r");
    fclose(fp);
}

void insertImage(pqxx::connection* conn, const std::string& image_path, const std::string& image_name){
    std::ifstream file(image_path, std::ios::binary);
    std::vector<unsigned char> buffer( (std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    
    pqxx::work txn(*conn);
    //std::string hex_data = "\\x" + pqxx::to_hex(buffer.data(), buffer.size());

    pqxx::binarystring blob(buffer.data(), buffer.size());
        txn.exec_params(
            "INSERT INTO images (name, image_data) VALUES ($1, $2)",
            image_name,
            blob
        );

        txn.commit();
        std::cout << "Image inserted successfully!" << std::endl;
        
        
}

void* handle_user(void* arg){
    int client_id = *((int*)arg);
    std::cout<< client_id <<std::endl;
    delete (int*)arg;
    char* message;
    client_count_mutex.lock();
    bool flag = 0;
    while(true){
        message = new char[1024];
        if(recv(connections[client_id], message, sizeof(message), NULL)){
            if (flag == 0){
                std::cout<<message<<std::endl;
                flag = 1;
                pqxx::connection conn("dbname=application user=postgres password=root hostaddr=127.0.0.1 port=5432");
                pqxx::work txn(conn);
                txn.exec_params(
                    "UPDATE users SET id = $1 WHERE name = $2",
                    client_id,
                    message
                ); //теперь id в таблице и номер в массиве совпадают

                txn.commit();
                client_count_mutex.unlock();
            }
            else{
                std::cout<<message<<std::endl;
                if(message[0] != '*'){ //отправим получателю команду на обновление диалога
                    pqxx::connection conn("dbname=application user=postgres password=root hostaddr=127.0.0.1 port=5432");
                    pqxx::work txn(conn);
                pqxx::result res = txn.exec_params(
                    "SELECT id FROM users WHERE name = $1",
                    message
                );
                int user_id = -1;
                if (!res.empty()) {
                    if(!(res[0][0].is_null())){
                        user_id = res[0][0].as<int>();
                    }
                }
                txn.commit();
                if (user_id != -1){
                    send(connections[user_id], "u", 1, 0);
                }
                }
                else{//если первый символ - звездочка, значит у нас картинка //мы получили message + && + имя юзера
                    std::cout<<"image generating"<<std::endl;
                    const char* n = strstr(message, "&&");
                    std::string name(n+2); //получили имя отправителя
                    std::string messagestr(message);
                    size_t pos = messagestr.find("&&");

                    std::string prefix = messagestr.substr(1, pos); 

                    //теперь мы имеем имя отправителя и его промпт
                    //задача - сделать уникальные названия
                    //название - отправительполучательвремя

                    //получим имя получателя
                    std::string in_dialog;

                    pqxx::connection conn("dbname=application user=postgres password=root hostaddr=127.0.0.1 port=5432");
                    pqxx::work txn(conn);
                pqxx::result res = txn.exec_params(
                    "SELECT in_dialog FROM users WHERE name = $1",
                    name
                );
                std::cout<<name<<std::endl;
                if (!res.empty()) {
                    if(!(res[0][0].is_null())){
                        in_dialog = res[0][0].as<std::string>();
                    }
                }
                txn.commit();
                    time_t now = time(0);  
                    char buffer[64];  
                    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));  

                    std::string buf(buffer);

                    std::string img_name = name + in_dialog + buf; //получили гига уникальное название

                    //сгененрируем саму картинку

                    image_mutex.lock();
                    GetToken();
                    GetImage(prefix, "../images/" + img_name);
                    image_mutex.unlock(); //тут сгенерили картинку

                    //осталось вставить ее в базу и обновить диалоги собеседникам

                    insertImage(&conn, "/home/sheleny_dmitry/cpp_chat_giga/images/"+ img_name +".jpg" , img_name);

                    //надо занести инфу в табличку сообщений -- с названием картинки
                    txn.exec_params(
                        "INSERT INTO messages (sender, reciever, type, content) VALUES ($1, $2, \'img\', $3)",
                        name,
                        in_dialog,
                        img_name
                    );

                    //теперь проверим, не ушли ли чуваки из диалога
                    //отправитель в диалоге?
                    
                    res = txn.exec_params(
                    "SELECT in_dialog FROM users WHERE name = $1",
                    name
                    );
                    if(!(res[0][0]).is_null()){
                    if (in_dialog == res[0][0].as<std::string>()) { //то отправим отправителю команду на обновление таблицы
                        pqxx::result res = txn.exec_params(
                        "SELECT id FROM users WHERE name = $1",
                        name
                        );
                        int user_id = -1;
                        if (!res.empty()) {
                            user_id = res[0][0].as<int>();
                        }
                        txn.commit();
                        if (user_id != -1){ //обновляйся
                            send(connections[user_id], "u", 1, 0);
                        }
                    }
                    
                    //аналогично для собутыльника
                    res = txn.exec_params(
                    "SELECT in_dialog FROM users WHERE name = $1",
                    in_dialog
                    );
                    if(!(res[0][0]).is_null()){
                        
                    
                    if (name == res[0][0].as<std::string>()) { //то отправим отправителю команду на обновление таблицы
                        pqxx::result res = txn.exec_params(
                        "SELECT id FROM users WHERE name = $1",
                        in_dialog
                        );
                        int user_id = -1;
                        if (!res.empty()) {
                            user_id = res[0][0].as<int>();
                        }
                        txn.commit();
                        if (user_id != -1){ //обновляйся
                            send(connections[user_id], "u", 1, 0);
                        }
                    }
                }
                    //ну вроде бы всё...

                }
                
            }
        }
        delete[] message;
        //memset(message, 0, sizeof(message));
        //message = "";
    }
}



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

    //теперь подключимся к базе данных

    pqxx::connection conn("dbname=application user=postgres password=root hostaddr=127.0.0.1 port=5432");
    if (conn.is_open()) {
        std::cout << "Opened database successfully: " << conn.dbname() << std::endl;
    } else {
        std::cout << "Can't open database" << std::endl;
        return 1;
    }

    //insertImage(&conn, "/home/sheleny_dmitry/cpp_chat_giga/output4.jpg", "output4");

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

    char connect_message[] = "welcome to GPTgram!";

    setvbuf(stdout, NULL, _IOLBF, 0);
    
    while(true){
        if((connect_soc = accept(listen_soc, nullptr, nullptr)) >= 0){
            connections[client_count] = connect_soc;
            std::cout<< "\nclient " << client_count << " connected"<< std::endl;
            int* client_id = new int(client_count);

            pthread_create(&thread, NULL, handle_user, (void*)client_id);
            //send(connect_soc, connect_message, sizeof(connect_message) - 1, 0);
            client_count++;

            
            //disconnect_client(client_count-1);
        }
        else {
            perror("Accept failed");
            continue;
        }
    }
     	
    conn.close();
    close(listen_soc);
    return 0;
}
