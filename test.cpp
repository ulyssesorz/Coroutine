#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<assert.h>
#include<iostream>
#include"coroutine.h"
using namespace std;

vector<int> co_ids(COROUTINE_SIZE, -1);

void setNoBlock(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    flag |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flag);
}

int socket_init()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(fd >= 0);
    int op = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8081);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int ret;
    ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    assert(ret >= 0);

    ret = listen(fd, 5);
    assert(ret >= 0);

    return fd;
}

void accept_conn(int fd, Schedule* s, function<void(Schedule*, void*)> cb)
{
    while(1)
    {
        int connfd = accept(fd, 0, 0);
        if(connfd > 0)
        {
            setNoBlock(connfd);
            int args[] = {connfd};
            int cid = s->coroutine_create(cb, args);
            assert(cid < COROUTINE_SIZE);
            co_ids[cid] = 1;
            s->coroutine_running(cid);
        }
        else
        {
            for(int i = 0; i < COROUTINE_SIZE; i++)
            {
                if(co_ids[i] == -1)
                    continue;
                s->coroutine_resume(i);
            }
        }
    }
}

void handle(Schedule* s, void* args)
{
    int* arr = (int*)args;
    int fd = arr[0];
    char buf[1024] = {0};
    int ret;

    while(1)
    {
        memset(buf, 0, sizeof(buf));
        ret = read(fd, buf, 1024);
        if(ret < 0)
        {
            s->coroutine_yield();
        }
        else if(ret == 0)
        {
            co_ids[s->curId()] = -1;
            break;
        }
        else
        {
            cout << "=> : " << buf;
            write(fd, buf, ret);
        }
    }
}

int main()
{
    Schedule* s = new Schedule();
    int fd = socket_init();
    setNoBlock(fd);
    accept_conn(fd, s, handle);
    return 0;
}