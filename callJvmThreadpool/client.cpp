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
    std::string plainsql, dbname, sendstream;
    int sock = 0;
    struct sockaddr_in serv_addr;

    plainsql = "select * from test";
    dbname = "student";
    sendstream = plainsql + "$" + dbname;
    char *sstr = (char*)sendstream.data();
    printf("sendstream is %s\n", sstr);
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

    send(sock, sstr, strlen(sstr), 0);
    printf("Hello message: %s sent\n", sstr);

    read(sock, buffer, 1024);
    printf("%s\n", buffer);


    close(sock);
}
