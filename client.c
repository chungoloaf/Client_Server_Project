#if defined(_WIN32)
#ifndef _WIN32_WINNI
#define _WIN32_WINNI 0X0600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#endif

#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())

#else
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(_WIN32)
#include <conio.h>
#endif
const int buffer = 1024;
size_t buffsize = 1024;

int main()
{
#if defined(_WIN32) // Helps initilation on windows system//
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d))
    {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

    printf("Initilize address \n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // TCP

    struct addrinfo *server_connect;
    char server_address[12];
    printf("What is severs IPV4 address? \n");
    scanf("%s", server_address);

    if (getaddrinfo(server_address, "8080", &hints, &server_connect))
    { // sets server IP and port
        fprintf(stderr, "Getinfo failed %d \n", GETSOCKETERRNO());
        return 1;
    }

    char addr_buff[100];

    getnameinfo(server_connect->ai_addr, server_connect->ai_addrlen, addr_buff, sizeof(addr_buff), 0, 0, NI_NUMERICHOST); // gets server IP and outputs it
    printf("Connecting to %s ", addr_buff);

    printf("creating Socket \n");
    SOCKET server_socket;
    server_socket = socket(server_connect->ai_family, server_connect->ai_socktype, server_connect->ai_protocol); // socket created
    if (!ISVALIDSOCKET(server_socket))
    {
        fprintf(stderr, "Socket creation failed %d \n", GETSOCKETERRNO());
        return 1;
    }

    printf("Connecting \n");
    if (connect(server_socket, server_connect->ai_addr, server_connect->ai_addrlen))
    { // connects client and server
        fprintf(stderr, "Connect failed %d \n", GETSOCKETERRNO());
        return 1;
    }

    freeaddrinfo(server_connect); // frees pointer

    fd_set master, reads;
    FD_ZERO(&master);
    FD_SET(server_socket, &master);
    int max_socket = server_socket;

    while (1)
    {
        fd_set reads;
        reads = master;
        char *user_choice = calloc(buffer, sizeof(char));
        printf("Send message?(y/n) \n");
        fgets(user_choice, buffer, stdin);
        user_choice = realloc(user_choice, strlen(user_choice)); // reallocates memory for buffer

        int i;

        if (*user_choice == 'y')
        {
            char *quit_message = (char *)calloc(buffer, sizeof(char)); // buffer for message
            quit_message = "quit\n";
            // quit_message = realloc(quit_message, strlen(quit_message));
            int bytes_sent = 0;
            free(user_choice);
            printf("Enter in a message or type quit to stop connection\n");
            char *message = (char *)calloc(buffer, sizeof(char)); // buffer for message
            fgets(message, buffer, stdin);
            message = realloc(message, strlen(message)); // reallocates memory for buffer

            int flag = strcmp(message, quit_message);
            if (flag == 0)
            {
                CLOSESOCKET(server_socket);
                exit(-1);
            }
            else
                bytes_sent = send(server_socket, message, strlen(message), 0);

            free(message);
        }

        else
        {
            free(user_choice);
            printf("Waiting for message \n");
            if (select(max_socket + 1, &reads, 0, 0, 0) < 0)
            {
                fprintf(stderr, "Select failed %d \n", GETSOCKETERRNO());
                return 1;
            }
            if (FD_ISSET(server_socket, &reads))
            {
                char *message_buffer = (char *)calloc(buffer, sizeof(char)); // creates buffer for echoed message
                printf("Reciving Message, \n");
                int bytes_recv = recv(server_socket, message_buffer, buffer, 0);  // received message
                message_buffer = realloc(message_buffer, strlen(message_buffer)); // realocates memory of buffe

                if (bytes_recv > 0)
                    printf("The message is: %s \nThe bytes recv is %d \n", message_buffer, bytes_recv);

                // sleep(1);
            }
        }
    }

    printf("Closing Connection \n"); // close connction
    CLOSESOCKET(server_socket);

    // free(message_buffer);

#if defined(_WIN32)
    WSACleanup();
#endif
    return 0;
}
/* For the cilent side the next thing I want to do is have the client be able to send and recive 
at the same time to do this the recv should be a new thread suing pthread create and and while loop
it will keep on reading up you close the conncetion then the thread will join to the akmin thread
as for the sending it will be the same thing just a sending fuction*/
