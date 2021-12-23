#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include <sys/wait.h>

enum errors {
    OK,
    ERR_INCORRECT_ARGS,
    ERR_SOCKET,
    ERR_CONNECT
};

int init_socket(const char *ip, int port) {
    //open socket, result is socket descriptor
    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Fail: open socket");
        _exit(ERR_SOCKET);
    }

    //prepare server address
    struct hostent *host = gethostbyname(ip);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    memcpy(&server_address.sin_addr, host -> h_addr_list[0], sizeof(server_address));

    //connection
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    memcpy(&sin.sin_addr, host->h_addr_list[0], sizeof(sin.sin_addr));
    if (connect(server_socket, (struct sockaddr*) &sin, sizeof(sin)) < 0) {
        perror("Fail: connect");
        _exit(ERR_CONNECT);
    }
    return server_socket;
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		puts("Incorrect args.");
		puts("./client <ip> <port>");
		puts("Example:");
		puts("./client 127.0.0.1 5000");
		return ERR_INCORRECT_ARGS;
	}
	char *ip = argv[1];
	int port = atoi(argv[2]);
	int server = init_socket(ip, port);
	char data;
	puts("Write data:");

	if(fork() == 0)
	{
		int j = 0;
		while(read(server, &data, 1) >= 0)
		{
	    		write(1, &data, 1);
		}
		exit(0);
	}
	char *word = NULL;
	int i = 0;
	while(read(0, &word[i], 1) >= 0)
	{

		if(word[i] == ' ' && word[i] == '\n')
		{
	    		write(server, "GET ", 4);
	    		write(server, word, i);
	    		write(server, " HTTP/1.1\nHost: mymath.info\n\n", 29);
	    		i = 0;
		}
		i++;
	}
	wait(NULL);
	close(server);
	return OK;
}
