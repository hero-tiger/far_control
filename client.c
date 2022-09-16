//
// Created by kobe1 on 8/28/22.
//
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include "sm4.h"
char readBuf[1024];
char msg[1024];
int socket_connect(char* ip,unsigned  short int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        return -1;
    }
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port);
    client_addr.sin_addr.s_addr = inet_addr(ip);
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *) &opt, sizeof(opt)) == -1) {
        fprintf(stderr,"failed to SO_REUSEADDR socket\n");
        return -1;
    }
    if (connect(fd, (struct sockaddr *) &client_addr, sizeof(struct sockaddr)) == -1  ) {
        fprintf(stderr,"socket connect error!\n");
        return -1;
    }
    return fd;
}

int client_port(char* ip,unsigned  short int port)
{
    char buf[1024];
    char temp[1024];
    int client_fd = socket_connect(ip,port);
    if (client_fd == -1) {
        fprintf(stdout,"client socket create failed\n");
        return 0 ;
    } else {
        fprintf(stdout,"socket connect success!\n");
    }
    while(1){
        printf("oprate: ");
        memset(buf,0,sizeof(buf));
        gets(buf);
        sm4e(buf,temp,strlen(buf));
        write(client_fd,temp,sizeof(temp));
        if(!strcmp(buf,"shell")){
            fcntl(client_fd,F_SETFL,0);
            int rc = fork();
            if(rc == 0){
                while(1){
                    memset(readBuf,0,sizeof(readBuf));
                    memset(temp,0,sizeof(temp));
                    int n_read = read(client_fd,readBuf,sizeof(readBuf));
                    if(n_read == -1){
                        perror("read");
                    }else if (n_read > 0){
                        sm4d(readBuf,temp,n_read);
                        printf("%s",temp);
                        fflush(stdout);
                    }
                }
            }
            while(1){
                usleep(9000);
                printf("$ ");
                memset(msg,0,sizeof(msg));
                gets(msg);
                sm4e(msg,temp,strlen(msg));
                write(client_fd,temp,sizeof (temp));
                if(!strcmp(msg,"exit")) {
                    kill(rc, SIGTERM);
                    break;
                }
            }
        }
        else if(!strcmp(buf,"upload"))
        {
            char file_name[1024];
            while(1)
            {
                memset(file_name,0,1024);
                fflush(stdin);
                printf("File Name:");
                fflush(stdin);
                gets(file_name);
                sm4e(file_name,temp,1024);
                write(client_fd,temp,1024);
                if(!strcmp(file_name,"exit"))
                {
                    break;
                }

                char file[1024];
                FILE *fd = fopen(file_name, "r");
                if(fd == NULL)
                {
                    printf(stderr,"No this files!");
                    fflush(stderr);
                }
                else{
                    int ret;
                    memset(file,0,sizeof(file));
                    fcntl(client_fd,F_SETFL,0);
                    while((ret = fread(file,1,1024,fd)) > 0)
                    {
                        write(client_fd,file,ret);
                        memset(file,0,sizeof(file));
                    }
                    fclose(fd);
                }

            }

        }
        else if(!strcmp(buf,"download"))
        {
            char file_name[1024];
            char buffer[1024];
            while(1)
            {
                memset(file_name,0,1024);
                fflush(stdin);
                printf("File Name:");
                gets(file_name);
                fflush(stdin);
                sm4e(file_name,temp,1024);
                write(client_fd, temp,1024);
                if(!strcmp(file_name,"exit"))
                {
                    break;
                }
                FILE *fp = fopen(file_name, "w");
                if (fp == NULL)
                {
                    printf("File: %s Can Not Open To Write!\n", file_name);
                }
                else{
                    memset(buffer,0, sizeof(buffer));
                    int length = 0;
                    fcntl(client_fd,F_SETFL,FNDELAY);
                    usleep(9000);
                    while( (length = read(client_fd, buffer, sizeof(buffer))) >=0 )
                    {
                        int write_length = fwrite(buffer, 1, length, fp);
                        memset(buffer,0, sizeof(buffer));
                    }
                    fclose(fp);
                    printf("Recieve File: %s From Server Finished!\n",file_name);
                }

            }
        }

    }
return 0;
}

