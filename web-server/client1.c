#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

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

char *scan_word() {
    char ch;
    char *word = NULL;
    int word_len = 0;

    ch = getchar();
    while (ch != '\n' && ch != ' ') {
        word = realloc(word, (word_len + 1) * sizeof(char));
        word[word_len] = ch;
        word_len++;
        ch = getchar();
    }
    word = realloc(word, (word_len + 1) * sizeof(char));
    word[word_len] = '\0';
    return word;
}


char *get_response(int server) {
    char *response = NULL;
    int response_len = 0;

    do {
        response = realloc(response, (response_len + 1) * sizeof(char));
        if (read(server, &response[response_len], 1) <= 0) {
            close(server);
            free(response);
            exit(0);
        }
        response_len++;
    } while (response[response_len - 1] != '\0');
    return response;
}

int check_argc(int argc) {
    if (argc != 3) {
        puts("Incorrect args.");
        puts("./client <ip> <port>");
        puts("Example:");
        puts("./client 127.0.0.1 5000");
        return 1;
    }
    return 0;
}

void server_recieve(int server) {
    char *response = NULL;

    while (1) {
        response = get_response(server);
        printf("Response:\n%s", response);
        free(response);
    }
    exit(0);
}


// void send_request(char *request, int server) {
//     size_t request_size = (strlen(request)) * sizeof(char);
//     printf("%s", request);
//     request[strlen(request)] = '\0';
//     printf("%ld\n", request_size);
//     if (write(server, &request[0], request_size) <= 0) {
//         close(server);
//         free(request);
//         exit(0);
//     }
// }


void send_request(char *request, int server) {
    int request_len = 0;
    printf("Request:\n%s", request);
    do {
        if (write(server, &request[request_len], 1) <= 0) {
            close(server);
            free(request);
            exit(0);
        }
        request_len++;
    } while (request[request_len - 1] != '\0');
}

void terminal_recieve(char *ip, int server) {
    char *filename = NULL;
    size_t filename_size;
    size_t ip_size;
    size_t request_size = 0;
    char *request = NULL;
    const char REQUEST_GET[] = {
        "GET %s HTTP/1.1\nHost: %s\n\n"
    };
    filename = scan_word();

    while (strcmp(filename, "exit") != 0) {
        filename_size = strlen(filename);
        ip_size = strlen(ip);
        request_size = sizeof(REQUEST_GET) + filename_size + ip_size;
        request = malloc(request_size);

        snprintf(request, request_size, REQUEST_GET, filename, ip);
        // printf("%s", request);
        send_request(request, server);
        free(filename);
        free(request);
        filename = scan_word();
    }
    free(filename);
    exit(0);
}

int main(int argc, char **argv) {
    if (check_argc(argc)) {
        return 1;
    }
    puts("Type \"exit\" to quit");

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int server = init_socket(ip, port);
    int pid;

    pid = fork();
    if (pid < 0) {
        perror("Error fork");
        exit(1);
    } else if (pid == 0) {
        server_recieve(server);
    } else {
        terminal_recieve(ip, server);
    }
    wait(NULL);
    close(server);
    return 0;
}
