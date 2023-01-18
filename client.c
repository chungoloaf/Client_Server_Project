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
#include <sys/time.h>
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
#include <signal.h>
#include <time.h>
#include <pthread.h>
#if defined(_WIN32)
#include <conio.h>
#endif

const int buffer = 1024;// buffers
size_t buffsize = 1024;
static volatile int keep_running =1;

struct socket_params{
    SOCKET socket_number; // struct for multithreading
};

void sighandle(){
    keep_running=0;
}

void getdir(void* context){// function for recv cpu info
    struct socket_params *struct_ptr=(struct socket_params*) context;
    char* message_buffer=calloc(buffer,sizeof(char));
    char *path_buffer=calloc(buffer,sizeof(char));
    recv(struct_ptr->socket_number,message_buffer,buffer,0);
    printf("%s",message_buffer);
    fgets(path_buffer, buffer, stdin);
    send(struct_ptr->socket_number,path_buffer,buffer,0);
    free(message_buffer);
    while(1){
        char* message_buffer=calloc(buffer,sizeof(char));
        recv(struct_ptr->socket_number, message_buffer, buffer,0);
        if(strcmp(message_buffer,"endword")==0){
            free(message_buffer);
            break;
        }
        else if(strcmp(message_buffer,"Path not found\n")==0){
            printf("%s\n", message_buffer);
            free(message_buffer);
            break;
        }
        printf("%s\n", message_buffer);
        free(message_buffer);
    }
    
    
    free(path_buffer);
    return;
}



void getcpuinfo(void* context){// function for recv cpu info
    struct socket_params *struct_ptr=(struct socket_params*) context;
    int* templines = malloc(sizeof(int)*buffer);
    int bytes_recv = recv(struct_ptr->socket_number, templines, buffer, 0); 
    int lines= *templines; 
    free(templines);
    
    
    for(int y=0; y< lines-1; y++){
        char* temp= calloc(buffer,sizeof(char));
        recv(struct_ptr->socket_number, temp, buffer, 0);
        printf("%s",temp);
        free(temp);}            
    
}


void *send_message(void* context){
    struct socket_params *struct_ptr=(struct socket_params*) context;
    int bytes_sent = 0;
    printf("Enter in a message or type quit to stop connection\n\n");
    char *message = (char *)calloc(buffer, sizeof(char)); // buffer for message
    fgets(message, buffer, stdin); // fgets stalling
    message = realloc(message, strlen(message)); // reallocates memory for buffer

    int flag = strcmp(message, "quit\n");
    if (flag == 0)
    {
        free(message);
        CLOSESOCKET(struct_ptr->socket_number);
        exit(-1);
    }
    else{
        send(struct_ptr->socket_number, message, strlen(message), 0);}
        
        
        signal(SIGPIPE, sighandle);
        
        if(strcmp(message,"cpuinfo\n")==0){
            getcpuinfo(struct_ptr);}
        else if(strcmp(message,"dir\n")==0){
            getdir(struct_ptr);}
        

    
    free(message);
    return NULL;
    }

void *recv_message(void* context){
            struct socket_params *struct_ptr=(struct socket_params*) context;
            printf("Waiting for message \n");
                char *message_buffer = (char *)calloc(buffer, sizeof(char)); // creates buffer for echoed message
                printf("Reciving Message, \n");
                int bytes_recv = recv(struct_ptr->socket_number, message_buffer, buffer, 0);  // received message
                message_buffer = realloc(message_buffer, strlen(message_buffer)); // realocates memory of buffe

                if (bytes_recv > 0){
                    printf("The message is: %s \nThe bytes recv is %d \n \n", message_buffer, bytes_recv);}

                free(message_buffer);
                return NULL;
            }

        

int main()
{
    pthread_t send_thread, recv_thread;
    struct socket_params my_socket;
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
    my_socket.socket_number=server_socket;
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

// use select for the reading of messages top ksee if there is any to be read if not return the thread
        
        fd_set master;
        FD_ZERO(&master);
        FD_SET(server_socket, &master);
        fd_set reads;
        struct timeval timeout;
        timeout.tv_sec=1;
        timeout.tv_usec=0; 
        
        while(keep_running){
            reads = master;
            
            if (select(server_socket+1, &reads, 0, 0, &timeout) < 0) { 
                fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
                return 1;
            }

            if(pthread_create(&send_thread, NULL, &send_message,
                    (void *)(intptr_t)&my_socket)==0){
                        //fprintf(stderr,"Sending Thread Started \n");
                         pthread_join(send_thread,NULL);
                }
                
            
        if (FD_ISSET(server_socket, &reads)) {
            if(pthread_create(&recv_thread, NULL, &recv_message,
                    (void *)(intptr_t)&my_socket)==0){
                        //fprintf(stderr,"Recv Thread Started \n");
                        pthread_join(recv_thread,NULL);
                }
            
    
        }
            
            FD_ZERO(&reads);  
        }
        

       
    

    printf(" Server Down Closing Connection \n"); // close connction
    CLOSESOCKET(server_socket);


#if defined(_WIN32)
    WSACleanup();
#endif
    return 0;
}

