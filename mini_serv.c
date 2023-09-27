#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

typedef struct s_serv{
    int clientId;
    int fd_max;
    int clientIds[65536];
    char *msg[65536];
    char rbuf[1025], wbuf[42];
    fd_set read_fds, write_fds, fds;
} t_serv;

t_serv serv = {0};

static int extract_message(char **buff, char **msg){
    if (!*buff)
        return 0;
    *msg = 0;
    char *newBuffer;
    int i = -1;
    while(*buff[++i]){
        if (*buff[i] == '\n'){
            newBuffer = calloc(1, sizeof(*newBuffer) * (strlen(*buff + i + 1) + 1));
            if (!newBuffer)
                return 0;
            strcpy(newBuffer, *buff + i + 1);
            *msg = *buff;
            (*msg)[i + 1] = 0;
            *buff = newBuffer;
            return 1;
        }
    }
    return 0;
}

static char *str_join(char *str, char *add){
    char *newstr = (char *)malloc(sizeof(char) * (strlen(add) + (str != NULL? strlen(str): 0)));
    if (!newstr)
        return (0);
    newstr[0] = 0;
    if (str)
        strcat(newstr, str);
    free(str);
    strcat(newstr, add);
    return newstr;
}

static void globalMessage(int user, char *msg){
    for (int i = 0; i <= serv.fd_max; i++)
        if(FD_ISSET(i, &serv.write_fds) && i != user)
            send(i, msg, strlen(msg), 0);
}

static void deliver(int user){
    char *msg;

    while(extract_message(&serv.msg[user], &msg))
    {
        sprintf(serv.wbuf, "client %d: ", serv.clientIds[user]);
        globalMessage(user, serv.wbuf);
        globalMessage(user, msg);
        free(msg);
        msg = NULL;
    }
}

static void fatal(){
    write(2, "Fatal error\n", 12);
    exit(1);
}

static void add_client(const int fd){
    serv.fd_max = fd > serv.fd_max? fd : serv.fd_max;
    serv.clientIds[fd] = serv.clientId++;
    serv.msg[fd] = NULL;
    FD_SET(fd ,&serv.fds);
    sprintf(serv.wbuf, "server: client %d just arrived\n", serv.clientIds[fd]);
    globalMessage(fd, serv.wbuf);
}

static void remove_socket(int fd){
    sprintf(serv.wbuf, "server: client %d just left\n", serv.clientIds[fd]);
    globalMessage(fd, serv.wbuf);
    free(serv.msg[fd]);
    serv.msg[fd] = NULL;
    FD_CLR(fd, &serv.fds);
    close(fd);
}

static int create_socket(){
    serv.fd_max = socket(AF_INET, SOCK_STREAM, 0);
    if (serv.fd_max < 0)
        fatal();
    FD_SET(serv.fd_max, &serv.fds);
    return (serv.fd_max);
}

int main(int argc, char **argv){
    if (argc != 2){
        write(2, "Wrong number of arguments\n", 26);
        return 1;
    }
    FD_ZERO(&serv.fds);
    int socket = create_socket();
    struct sockaddr_in serveraddr;
    bzero(&serveraddr, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(atoi(argv[1]));

    if (bind(socket, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) != 0)
        fatal();
    if (listen(socket, 10) != 0)
        fatal();
    while(42){
        serv.read_fds = serv.write_fds = serv.fds;
        if (select(serv.fd_max + 1, &serv.read_fds, &serv.write_fds, NULL, NULL) < 0)
            fatal();
        for (int fd = 0; fd <= serv.fd_max; fd++){
            if (!FD_ISSET(fd, &serv.read_fds))
                continue;
            if (fd == socket){
                socklen_t addr_len = sizeof(serveraddr);
                int client =  accept(socket, (struct sockaddr *)&serveraddr, &addr_len);
                if (client >= 0){
                    add_client(client);
                    break;
                }
                continue;
            }
            int readed = recv(fd, serv.rbuf, 1024, 0);
            if (readed <= 0){
                remove_socket(fd);
                break;
            }
            serv.rbuf[readed] = 0;
            serv.msg[fd] = str_join(serv.msg[fd], serv.rbuf);
            deliver(fd);
        }
    }
    return 0;
}