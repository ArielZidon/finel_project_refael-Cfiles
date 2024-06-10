#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "arg.h"

int parseRead(char buffer_sent[MAXLINE]);
int parsewrite(char buffer_sent[MAXLINE]);
int ackFromServer(int sockfd,struct sockaddr_in cliaddr,int len);
int readFromServer(int sockfd,struct sockaddr_in cliaddr,int len);

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buffer_sent[MAXLINE];
    char buffer_recv[MAXLINE];
    int len, n, linelog;
    int status;

    /* Creating socket file descriptor */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* Filling server information */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    memset(&cliaddr, 0, sizeof(cliaddr));

    while (1) {
        printf("Enter message: ");
        fgets(buffer_sent, MAXLINE, stdin);

        if (strncmp(buffer_sent,"read",4) == 0) { 
            status = parseRead(buffer_sent);
            if(status){
                printf("wrong syntax!\n");
                continue;
            }
            sendto(sockfd, (const char *)buffer_sent, strlen(buffer_sent), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
            status = readFromServer(sockfd,cliaddr,len);
            if(status)
                continue;
        }

        if(strncmp(buffer_sent,"write",5) == 0){
            status = parsewrite(buffer_sent);
            if(status){
                printf("wrong syntax!!\n");
                continue;
            }
            sendto(sockfd, (const char *)buffer_sent, strlen(buffer_sent), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
            status = ackFromServer(sockfd,cliaddr,len);
            if(status){
                printf("bad recive from the server!\n");
                continue;
            }
        }

        if(strncmp(buffer_sent,"exit",4) == 0){
            sendto(sockfd, (const char *)buffer_sent, strlen(buffer_sent), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        }
    }
    close(sockfd);
    return 0;
}

int parseRead(char buffer_sent[MAXLINE]){
    int lines;
    /* sscanf() -> if %d - the number need to be sent is not a number, status will uneq to 1.. we check here if what we sent its a number */
    int status = sscanf(buffer_sent,"read %d",&lines); 
    if(status != 1)
        return 1;
    if(lines <= 0 || lines > 8)
        return 1;
    return 0;
}

int parsewrite(char buffer_sent[MAXLINE]){
    char buf [MAXLINE];
    int status = sscanf(buffer_sent,"write %s",buf); //check also if buf > 64 len
    if(status != 1)
        return 1;
    return 0;
}

int ackFromServer(int sockfd,struct sockaddr_in cliaddr,int len){
    char ackBuffer [MAXLINE]; 
    int n = recvfrom(sockfd, (char *)ackBuffer, MAXLINE, 0, (struct sockaddr *)&cliaddr, &len);
    if(n <= 0){
        return 1;
    }
    ackBuffer[n] = '\0';
    printf("Message sent.\n");
    printf("Received log: %s\n", ackBuffer);
    return 0;
}

int readFromServer(int sockfd,struct sockaddr_in cliaddr,int len){
    masseges msgs;
    int n = recvfrom(sockfd, (struct masseges *)&msgs, sizeof(struct masseges), 0, (struct sockaddr *)&cliaddr, &len);
    if(n <= 0){
        printf("bad recive from server - readFromServer!!\n");
        return 1;
    }
    for (int i = 0; i < msgs.lines; i++){
        printf("%d. %s\n",i,msgs.msg[i]);
    }
    return 0;
}
