//
// Created by wanhui on 11/2/19.
//

#include <cstdio>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#define PORT 8080

int main()
{
    std::string plainsql, dbname;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;

    plainsql = "select * from test;";
    char *psql = (char*)plainsql.data();
    dbname = "student";
    char *dbn = (char*)dbname.data();
    char split = '$';
    char buffer[1024] = {0};

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // convert IPV4 and IPV6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        printf("\n Invalid address/ Address not supported\n");
        return -1;
    }

    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed\n");
        return -1;
    }

    send(sock, psql, strlen(psql), 0);
    printf("Hello message: %s sent\n", psql);
    send(sock, reinterpret_cast<const void *>(split), 1, 0);
    send(sock, dbn, strlen(dbn), 0);
    printf("Talk message: %s send\n", dbn);

    valread = read(sock, buffer, 1024);
    printf("%s\n", buffer);
}