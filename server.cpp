/*======================================================
    > File Name: server.c
    > Author: lyh
    > E-mail:  
    > Other :  
    > Created Time: 2015年08月07日 星期五 16时09分02秒
 =======================================================*/

#include<iostream>
#include<string>
#include<stdio.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<pthread.h>
#include<json/json.h>

using namespace std;

void* recv_thread(void* arg)
{
    int* sock=(int*)arg;
    int   ret = 0;
    char  buf[1024];
    while(1)
    {
        bzero(buf,1024);
        ret = recv(*sock,buf,1024,0);
        if(ret<=0)
        {
            close(*sock);
            pthread_exit(0);
        }
        buf[strlen(buf)] = '\0';
        printf("recv:\n%s",buf);
//        printf("ret = %d\n",ret);
    }
}

void* send_thread(void* arg)
{
    int*  sock = (int*)arg;
    char  buf[1024];
    while(1)
    {
        bzero(buf,1024);
        printf("send:\n");
        scanf("%s",buf);
        send(*sock,buf,strlen(buf),0);
    }
}

void* sfile_thread(void* arg)
{
    int* sock = (int*)arg;
    char buf[32768];
    bzero(buf,32768);
    //接收下载文件请求
    cout<<recv(*sock,buf,1024,0)<<endl;
    buf[strlen(buf)] = '\0';
    Json::Value   json;
    Json::Reader  reader;
    int           mark;
    string        md5;
    if(reader.parse(buf,json))
    {
        mark = json["mark"].asInt();
        md5 = json["md5"].asString();
        cout<<"mark: "<<mark<<"  md5: "<<md5<<endl;
    }
    string  file = "./File/"+md5;
    int  fd = open(file.c_str(),O_RDONLY);
    struct stat   st;
    fstat(fd,&st);
    if(fd<0)
    {
        printf("打开文件出错\n");
        close(fd);
        pthread_exit(0);
    }
    cout<<"文件大小："<<st.st_size<<endl;
    int            ret=0;
    long long      sum = 0;
    bzero(buf,32768);
    while((ret=read(fd,buf,32768))>0)
    {
        ret = send(*sock,buf,32768,0);
        if(ret<0)
        {
            printf("发送失败\n");
            break;
        }
        sum  = sum+ret;
        printf("ret= %d, sum= %lld\n",ret,sum);
    }
    if(sum>=st.st_size)
    {
        cout<<"sum="<<st.st_size<<endl;
        printf("文件发送成功\n");
    }
    else
    {
        printf("文件发送不完整\n");
    }
    close(fd);
    pthread_exit(0);
}

void* rfile_thread(void* arg)
{
    int* sock = (int*)arg;
    char buf[32768];
    bzero(buf,32768);
    //接收上传文件请求
    cout<<recv(*sock,buf,1024,0)<<endl;
    cout<<"json:"<<buf<<endl;
    buf[strlen(buf)] = '\0';
    Json::Value   json;
    Json::Reader  reader;
    int           mark;
    string        md5;
    string        sizebuf;
    long long           size;
    if(reader.parse(buf,json))
    {
        mark = json["mark"].asInt();
        md5 = json["md5"].asString();
        sizebuf = json["Size"].asString();
        cout<<"mark: "<<mark<<"  md5: "<<md5<<endl;
    }
    string  file = "./File/"+md5;
    int  fd = open(file.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0666);

    if(fd<0)
    {
        printf("保存文件出错\n");
        close(fd);
        pthread_exit(0);
    }
   
    size = atoll(sizebuf.c_str());
    cout<<"文件大小："<<size<<endl;
    int          ret=0;
    long long    sum=0;
    memset(buf,'\0',sizeof(buf));
    sleep(1);
    while((ret=recv(*sock,buf,32768,0))>0)
    {
        if(sum+ret>=size)
        {
            ret = write(fd,buf,size-sum);
        }
        else
        {
            ret = write(fd,buf,32768);
        }
        if(ret<0)
        {
            printf("写入文件失败\n");
            break;
        }
        sum  = sum+ret;
        printf("ret= %d, sum= %lld\n",ret,sum);
        memset(buf,'\0',sizeof(buf));
        if(sum>=size)
        {
            printf("接收完成\n");
            break;
        }
    }
 
    close(fd);
    pthread_exit(0);
}

int main(int argc,char* argv[])
{
    pthread_t   thid1,thid2,thid3,thid4;

	if(argc <= 2)
	{
		printf("usage: %s ip port\n",basename(argv[0]));  //未知的warning
		return 1;
	}
	const char*   ip = argv[1];
	int   port = atoi(argv[2]);
 
	struct sockaddr_in    address;
	bzero(&address,sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET,ip,&address.sin_addr);
	address.sin_port = htons(port);

	int  sock = socket(PF_INET,SOCK_STREAM,0);
	assert(sock >= 0);

	int ret = bind(sock,(struct sockaddr*)&address,sizeof(address));
	assert(ret != -1);

	ret = listen(sock,5);
	assert(ret != -1);

    int    k=0;
    printf("^^^^^^^^^^^^^^^^^^^^^^^\n");
    printf("1、其他处理\n");
    printf("2、发送文件\n");
    printf("3、接收文件\n");
    printf("-----------------------\n");
    cout<<"请选择：";
    scanf("%d",&k);   getchar();


	struct sockaddr_in   client;
	socklen_t client_addrlength = sizeof(client);
    while(1)
    {
        int connfd = accept(sock,(struct sockaddr*)&client,&client_addrlength);
        if(connfd<0)
        {
            printf("errno is: %d\n",errno);
        }
      
        switch(k)
        {
            case 1:
                pthread_create(&thid1,NULL,recv_thread,&connfd);
                pthread_create(&thid2,NULL,send_thread,&connfd);
                break;
            case 2:
                pthread_create(&thid3,NULL,sfile_thread,&connfd);
                break;
            case 3:
                pthread_create(&thid4,NULL,rfile_thread,&connfd);
                break;
        }
        
    }
	close(sock);
	return 0;
}
