#include <iostream>
#include <stdio.h>
#include <cstring>
#include <string>

std::string token;

std::string auth_key = "";

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

int main() {
    GetToken();
    std::cout<<std::endl;
    std::cout<<token<<std::endl;

    GetImage("нарисуй мне . Реализм 4к", "output");

}