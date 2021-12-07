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
    ERR_CONNECT
};

int init_socket(const char *ip, int port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct hostent *host = gethostbyname(ip);
    struct sockaddr_in server_address;

    //open socket, result is socket descriptor
    if (server_socket < 0) {
        perror("Fail: open socket");
        exit(ERR_SOCKET);
    }

    //prepare server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    memcpy(&server_address.sin_addr, host -> h_addr_list[0],
           (socklen_t) sizeof server_address.sin_addr);

    //connection
    if (connect(server_socket, (struct sockaddr*) &server_address,
        (socklen_t) sizeof server_address) < 0) {
        perror("Fail: connect");
        exit(ERR_CONNECT);
    }
    return server_socket;
}

int main(int argc, char **argv)
{
        char *ip;
        int port, server;
        char data;

        if(argc != 3)
        {
                puts("Incorrect args.");
                puts("./client <ip> <port>");
                puts("Example:");
                puts("./client 127.0.0.1 5000");
                return ERR_INCORRECT_ARGS;
        }
        ip = argv[1];
        port = atoi(argv[2]);
        server = init_socket(ip, port);
        puts("Recieve data:");
        while(read(0, &data, 1) >= 0)
        {
                if(write(server, &data, 1) < 0)
                {
                        return 1;
                }
        }
        /*do
        {
                //printf("Message: ");
                scanf("\n%200[0-9a-zA-Z.,! ]", data);
                send(server, data, sizeof(data), 0);
                //read(server, data, sizeof data);

                //write(server, data, sizeof data);
                //recv(server, data, sizeof(data), 0);
                //printf("%s\n", data);
        }
        while(1);*/
        puts("");
        close(server);
        return OK;
}
