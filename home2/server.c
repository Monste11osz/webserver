include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

enum errors {
    OK,
    ERR_INCORRECT_ARGS,
    ERR_SOCKET,
    ERR_SETSOCKETOPT,
    ERR_BIND,
    ERR_LISTEN
};

int init_socket(int port) {
    int server_socket, socket_option = 1;
    struct sockaddr_in server_address;

    //open socket, return socket descriptor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Fail: open socket");
        exit(ERR_SOCKET);
    }

    //set socket option
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &socket_option, (socklen_t) sizeof socket_option);
    if (server_socket < 0) {
        perror("Fail: set socket options");
        exit(ERR_SETSOCKETOPT);
    }

    //set socket address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_socket, (struct sockaddr *) &server_address, (socklen_t) sizeof server_address) < 0) {
        perror("Fail: bind socket address");
        exit(ERR_BIND);
    }

    //listen mode start
    if (listen(server_socket, 5) < 0) {
        perror("Fail: bind socket address");
        exit(ERR_LISTEN);
    }
    return server_socket;
}


int main(int argc, char** argv)
{
        int port, server_socket, *client_socket;
        struct sockaddr_in client_address;
        socklen_t size = sizeof client_address;
        char buf;
        int chela;

        if(argc != 3)
        {
                puts("Incorrect args.");
                puts("./server <port>");
                puts("Example:");
                puts("./server 5000");
                return ERR_INCORRECT_ARGS;
        }
        port = atoi(argv[1]);
        chela = atoi(argv[2]);
        server_socket = init_socket(port);
        client_socket = malloc(chela * sizeof(int));
        puts("Wait for connection");
        for(int i = 1; i <= chela; i++)
        {
                client_socket[i] = accept(server_socket, (struct sockaddr *) &client_address, &size);
        }
        for(int i = 1; i <= chela; i++)
        {
                if(fork() == 0)
                {
                        for(int j = 1; j <= chela; j++)
                        {
                                if(i != j)
                                        close(client_socket[j]);
                        }
                        do
                        {
                                printf("%d %c\n", i, buf);
                        }
                        while(read(client_socket[i], &buf, 1) > 0);
                        close(client_socket[i]);
                        return 1;
                }
                //printf("connected: %s %d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                //puts("Send buf:");
                /*while(recv(client_socket, buf, sizeof(buf), 0) > 0)
                {
                        printf("%s\n", buf);
                        printf("Message: ");
                        scanf("\n%200[0-9a-zA-Z.,! ]", buf);
                        send(client_socket, buf, sizeof(buf), 0);
                }*/
        }
        for(int i = 1; i <= chela; i++)
        {
                close(client_socket[i]);
        }
       	//puts(buf);
        return OK;
}
