#define SERVER_IP "10.0.0.1" // Server IP address
#define CLIENT_IP "10.0.0.2"  // my client ip - mykernel
#define PORT 8080
#define MAXLINE 64

typedef struct masseges{
    char msg [8] [MAXLINE];
    int lines;
}masseges , *pmasseges;
