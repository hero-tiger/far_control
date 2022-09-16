#include <stdio.h>
#include "server.h"
#include "client.h"
#include <stdlib.h>
#include <unistd.h>
int main(int argc , char* argv[])
{
    if(argc == 2)
    {
        server_port(atoi(argv[1]));
    }
    if(argc == 3)
    {
        client_port(argv[1],atoi(argv[2]));
    }
}
#include<stdlib.h>
#include<string.h>
//int main()
//{
//    int fd[2];
//    pipe(fd);
//    char buf[1024]="ls -l";
//    char readbuf[1024];
//    for(int i = 0;i<3;i++){
//        fork();
//        system("/bin/sh");
//        memset(buf,0,1024);
//        write(fd[1],buf,1024);
//        if(getpid()== 0)
//        {
//            memset(readbuf,0,1024);
//            read(fd[0],readbuf,1024);
//            printf("%s",readbuf);
//        }
//    }
//    return 0;
//}