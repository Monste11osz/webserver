#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <sys/sendfile.h>
#define CLIENTS_NUM 5

typedef struct {
 char *ext;
 char *mediatype;
} extn;

extn extensions[] = {
    {".txt", "text/plain"},
    {".jpg", "image/jpg" },
    {0,0}
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

int isEndLine(char ch) {
    return ch == '\r';
}

int isEmptyChar(char ch) {
    return ch == '\n';
}

int readStr(int fd, char **string)
{
    	char *str = NULL;
    	char ch;
    	int i = 0, status;
    	while (1)
	{
        	status = read(fd, &ch, sizeof(char));
        	if(status <= 0) {
        		break;
        	}
        	if(isEndLine(ch)) {
            		break;
        	}
        	if(isEmptyChar(ch)) {
            		continue;
        	}
        	str = realloc(str, (i + 1) * sizeof(char));
        	str[i] = ch;
        	i++;
    }
    str = realloc(str, (i + 1) * sizeof(char));
    str[i] = '\0';
    *string = str;
    return status;
}

int getFileSize(int fd)
{
    struct stat stat_struct;
    if (fstat(fd, &stat_struct) == -1)
        return (1);
    return (int) stat_struct.st_size;
}

int run_binary(char *path, int client_socket, char **argv) {
    if (fork() == 0) {
        dup2(client_socket, 1);
        close(client_socket);
        if(execv(path, argv) < 0) {
            perror("exec error");
            exit(1);
        }
        exit(0);
    }
    wait(NULL);
    return 0;
}

char** split(char *str, char *delimiter) {
    int i = 0;
    char  *strtokAnswer = NULL;
    char **words = NULL;
    strtokAnswer = strtok(str, delimiter);
    while(strtokAnswer != NULL) {
        words = realloc(words, (i + 1) * sizeof(char *));
        words[i] = strtokAnswer;
        strtokAnswer = strtok(NULL, delimiter);
        i++;
    }
    words = realloc(words, (i + 1) * sizeof(char *));
    words[i] = NULL;
    return words;
}

char** returnArgv(char *arr[], int client_socket, char *postStr) {
    char *del = "=&";
    int i = 0;
    char *get_post_Arr[2] = {"GET", "POST"};
    int argvIndex = 1, queryArrIndex = 0;
    char **argv = NULL;
    char **queryArr = NULL;
    char *query = NULL;
    if ((strcmp(arr[0], get_post_Arr[0])) != 0 &&
        (strcmp(arr[0], get_post_Arr[1]) != 0)) {
        sendError(client_socket);
        exit(1);
    }
    argv = realloc(argv, (argvIndex + 1) * sizeof(char *));
    argv[0] = arr[1];
    while (i != 2) {
        if (strcmp(arr[0], get_post_Arr[i]) == 0) {
            if (i == 0) {
                if ((query = strchr(arr[1], '?')) != NULL) {
                    query[0] = '\0';
                    query++;
                }
                queryArr = split(query, del);
            }
            if (i == 1) {
                queryArr = split(postStr, del);
            }
            while(queryArr[queryArrIndex] != NULL) {
                argv = realloc(argv, (argvIndex + 1) * sizeof(char *));
                argv[argvIndex] = queryArr[queryArrIndex];
                puts(argv[argvIndex]);
                queryArrIndex++;
                argvIndex++;
            }
            argv = realloc(argv, (argvIndex + 1) * sizeof(char *));
            argv[argvIndex] = NULL;
        }
        i++;
    }
    return argv;
}

char* defineType(char *filename) {
    char *type = NULL;
    int i = 0;

    if ((type = strchr(filename, '.')) == NULL) {
        type = "bin";
        return type;
    }
    while (extensions[i].ext != NULL) {
        if (strstr(filename, extensions[i].ext) != NULL) {
            type = extensions[i].mediatype;
            return type;
        }
        i++;
    }
    return NULL;
}

int requestAnswer(char *arr[], int client_socket, char *postStr) {
    struct stat stat;
    char **argv = NULL;
    char *type = NULL;
    char *string = NULL;
    off_t offset = 0;
    int fd = 0, len = 0, i = 0;
    char ch;

    argv = returnArgv(arr, client_socket, postStr);
    type = defineType(arr[1]);
    if (type == NULL) {
        sendError(client_socket);
        return -1;
    }
    if (strcmp(arr[2], strForHTTP) != 0) {
        sendError(client_socket);
        return -1;
    }
    if (arr[1][0] == '/') {
        arr[1]++;
    }
    fd = open(arr[1], O_RDONLY);
    if (fd < 0) {
        perror("open error");
        sendError(client_socket);
        return -1;
    }
    lstat(arr[1], &stat);
    len = getFileSize(fd);
    printf("%d\n", len);
    if (S_ISDIR(stat.st_mode)) {
        sendError(client_socket);
        return -1;
    }
    sendHeader(client_socket, type, len);
    if (strcmp(type, "bin") == 0) {
        run_binary(arr[1], client_socket, argv);
    } else {
        while (extensions[i].ext != NULL) {
            if (strcmp(type, extensions[i].mediatype) == 0) {
                sendfile(client_socket, fd, &offset, len);
                break;
            }
            i++;
        }
    }
    close(fd);
    puts("closing socket");
    close(client_socket);
    return 0;
}



void interactionWithClient(int client_socket) {
    char **arr = NULL, **words;
    char *str = NULL;
    char *del = " ";
    char ch;
    char *postStr = NULL;
    int ret = 0, strings = 0;
    while (1) {
        while (1) {
            ret = readStr(client_socket, &str);
            if (ret <= 0) {
                puts("shutting down");
                return;
            }
            printf("%d:", strings);
            if (strcmp(str, "") == 0) {
                read(client_socket, &ch, 1);
                fcntl(client_socket, F_SETFL, O_NONBLOCK);
                readStr(client_socket, &postStr);
                fcntl(client_socket, F_SETFL, !O_NONBLOCK);
                break;
            }
            puts(str);
            arr = realloc(arr, (strings + 1) * sizeof(char *));
            arr[strings] = str;
            strings++;
            str = NULL;
        }
        if (arr != NULL) {
            words = split(arr[0] , del);
            requestAnswer(words, client_socket, postStr);
            for(int i = 0; i < strings; i++) {
                free(arr[i]);
            }
            free(arr);
            arr = NULL;
        }
    }
}

int main(int argc, char** argv) {
    int port, server_socket;
    struct sockaddr_in client_address;
    socklen_t size = sizeof client_address;
    if (argc != 2) {
        puts("Incorrect args.");
        puts("./server <port>");
        puts("Example:");
        puts("./server 5000");
        return ERR_INCORRECT_ARGS;
    }
    port = atoi(argv[1]);
    server_socket = init_socket(port);
    int clientNum = 0;
    pid_t pids[CLIENTS_NUM] = {0}, pid;
    int client_socket;
    while (1) {
        for (clientNum = 0; clientNum < CLIENTS_NUM; clientNum++) {
            if (pids[clientNum] == 0) {
                break;
            }
        }
        if (pids[clientNum] == 0) {
            pids[clientNum] = fork();
            if (pids[clientNum] == 0) {
                client_socket = accept(
                    server_socket,
                    (struct sockaddr *) &client_address,
                    &size);
                printf("client connected, slot number: %d\n", clientNum);
                interactionWithClient(client_socket);
                return OK;
            }
        } else {
            pid = wait(NULL);
            for (int i = 0; i < CLIENTS_NUM; i++) {
                if (pids[i] == pid) {
                    printf("slot %d has freed\n", i);
                    pids[i] = 0;
                    break;
                }
            }
            for (int i = 0; i < CLIENTS_NUM; i++) {
                if (pids[i] != 0) {
                    if (waitpid(pids[i], NULL, WNOHANG) > 0) {
                        printf("slot %d has freed\n", i);
                        pids[i] = 0;
                        break;
                    }
                }
            }
        }
    }
    return OK;
}
