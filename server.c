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
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#if defined(_WIN32)
#include <conio.h>
#endif
#include <ctype.h>



int BUFFER=1024, waitlist=15;
int msleep(long tms)
{
    struct timespec ts;
    int ret;

    if (tms < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = tms / 1000;
    ts.tv_nsec = (tms % 1000) * 1000000;

    do {
        ret = nanosleep(&ts, &ts);
    } while (ret && errno == EINTR);

    return ret;
}

struct socket_params{
    int socket_number;
    int host_socket;
    int message_bytes;
    char* message_buffer;
    SOCKET my_max_socket;
    fd_set master;
    char ** my_address;
};


void getcpuinfo(void* context){
    struct socket_params *struct_ptr=(struct socket_params*) context;
    char c;
    FILE *fptr;
    int MAX_LINES= 500;
    int  MAX_LEN= 1000;


    char ** cpudata=malloc(sizeof(char*)*MAX_LINES);
    for(int x=0; x<MAX_LINES ; x++){
        cpudata[x]=malloc(sizeof(char)*MAX_LEN);
    }

    fptr= fopen("/proc/cpuinfo", "r");
    if (fptr == NULL){
        printf("Error opening file \n");
    }
    
    else{
       
        int line=0;
        while (!feof(fptr) && !ferror(fptr))
    {
        if(fgets(cpudata[line], MAX_LEN, fptr) != NULL){
            line++;
        }
        
        
    }
    
    int *lineptr= &line;
    SOCKET j;
    printf("Sending CPU information to client %d \n", struct_ptr->socket_number);

                   for (j = 1; j <= struct_ptr->my_max_socket; ++j) {
                        if (FD_ISSET(j, &struct_ptr->master)) {
                            if ( j == struct_ptr->socket_number){
                                send(j, &line, sizeof(line),0);
                                for(int x=0; x<line; x++){
                                    send(j, cpudata[x], MAX_LEN,0);
                                    msleep(10*7 );}
                                    
                                }
                            else
                                 continue;
                               }
}         
    }

                


    for (int x =0; x<MAX_LINES; x++){
        free(cpudata[x]);
        cpudata[x]=NULL;
    }
    free(cpudata);
    cpudata=NULL;
    fclose(fptr);
}


void *message_handler(void* context){
    
    struct socket_params *struct_ptr=(struct socket_params*) context;
                SOCKET j;
                   for (j = 1; j <= struct_ptr->my_max_socket; ++j) {
                        if (FD_ISSET(j, &struct_ptr->master)) {
                            if (j == struct_ptr->host_socket || j == struct_ptr->socket_number)
                                continue;
                            else

                                send(j, struct_ptr->message_buffer, struct_ptr->message_bytes,0);
                        }
                    }
                    return NULL;
}


void *fd_handler(void* context){
                    struct socket_params *struct_ptr=(struct socket_params*) context;
                    struct sockaddr_storage client_address;
                    socklen_t client_len = sizeof(client_address);
                    SOCKET socket_client = accept((int)struct_ptr->host_socket,
                            (struct sockaddr*) &client_address,&client_len);
                    if (!ISVALIDSOCKET(socket_client)){
                        fprintf(stderr, "accept() failed. (%d)\n",
                                GETSOCKETERRNO());
                        EXIT_FAILURE;
                    }
                    char welcome_message[30]="Welcome to the chat server \n";
                    send(socket_client, welcome_message, strlen(welcome_message), 0);

    
                    FD_SET(socket_client,&struct_ptr->master);
                    if (socket_client > struct_ptr->my_max_socket)
                        struct_ptr->my_max_socket = socket_client;
                    
                    char *address_buffer=calloc(BUFFER,sizeof(char));
                    getnameinfo((struct sockaddr*)&client_address,
                            client_len,
                            address_buffer, BUFFER, 0, 0,
                            NI_NUMERICHOST);
                    printf("New connection from %s its its client number is %d \n", address_buffer, socket_client);
                    struct_ptr->my_address=&address_buffer;
                    
            return NULL;

}

int main() {
pthread_t connection_thread[waitlist], message_thread[15],fd_thread[15];
struct socket_params my_socket;
#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif


    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo("10.0.0.185", "8080", &hints, &bind_address);


    printf("Creating socket...\n");
    SOCKET socket_listen; 
    socket_listen = socket(bind_address->ai_family,
            bind_address->ai_socktype, bind_address->ai_protocol);
    my_socket.host_socket=socket_listen;
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    int yes=1;
    if(setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR,// helps with timeout issues on binding socket
    (void*)&yes,sizeof(yes))<0){
        fprintf(stderr, "Setsockopt failed %d \n", GETSOCKETERRNO());
    }

    printf("Binding socket to local address...\n");
    if (bind(socket_listen,
                bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(bind_address);


    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    FD_ZERO(&my_socket.master);
    FD_SET(socket_listen, &my_socket.master);
    my_socket.my_max_socket = socket_listen;

    printf("Waiting for connections...\n");


    while(1) {
        fd_set reads;
        reads = my_socket.master;
        if (select(my_socket.my_max_socket+1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        SOCKET i;
        for(i = 1; i <= my_socket.my_max_socket; ++i) {
            my_socket.socket_number=i;
            if (FD_ISSET(i, &reads)) {
                if (i == socket_listen) {
                    if(pthread_create(&fd_thread[i], NULL, &fd_handler,
                    (void *)(intptr_t)&my_socket)==0){
                        //fprintf(stderr,"Connection Handler Started \n");
                }
                char *my_address=calloc(BUFFER,sizeof(char));
                pthread_join(fd_thread[i], NULL);

                }
            

                 else { // this is for clients send messages it will multithread it
                   char* read=calloc(BUFFER,sizeof(char));
                    my_socket.message_buffer=read;
                    my_socket.message_bytes = recv(i, read, BUFFER, 0);
                    read=realloc(read,strlen(read));
                    
                  
                    printf("Client %d message is: %s \n",i, my_socket.message_buffer);
                        if(strcmp(my_socket.message_buffer, "cpuinfo\n")==0){
                            getcpuinfo(&my_socket);}
                            
                    
                        else{
                        if(pthread_create(&message_thread[i], NULL, &message_handler,
                        (void *)(intptr_t)&my_socket)==0){
                            //fprintf(stderr,"Message Handler Started \n");
                            pthread_join(message_thread[i],NULL);
                            }    
                        }

                        if (my_socket.message_bytes < 1) {
                            printf("Closing connection from %d \n", i);
                            FD_CLR(i, &my_socket.master);
                            CLOSESOCKET(i);
                            continue;
                        }
                        free(read);
                        
                        
                        
                    }
            } //if FD_ISSET
        } //for i to max_socket
    } //while(1)



    printf("Closing listening socket...\n");
    CLOSESOCKET(socket_listen);

#if defined(_WIN32)
    WSACleanup();
#endif


    printf("Finished.\n");

    return 0;
}

