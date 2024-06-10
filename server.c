#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "arg.h"
#include <pthread.h>

int parseRead(char buffer_sent[MAXLINE]);
void printToLog(FILE *fp,char * buffer);
int check_sendToUser(FILE *fp, int sockfd ,struct sockaddr_in cliaddr,int len,char buffer[MAXLINE],masseges msgs);
pthread_mutex_t readLock;
pthread_mutex_t writeLock;

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    FILE *fp;
    char buffer[MAXLINE];
    char * recvbuf = "the server recive your message";
    int len, n;
    int status; //sendto() status
    masseges msgs;

    /* Creating socket file descriptor */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { //AF_INET - apv4 x.y.z.w  SOCK_DGRAM-udp
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    /*we init the structs (this is the packet that udp use to work)*/
    memset(&servaddr, 0, sizeof(servaddr));  

    /* Filling server information */
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP); /* convert the string pointed in the standard IPv4 -> use as an Internet address. */
    servaddr.sin_port = htons(PORT); // function converts the unsigned integer hostlong from host byte order to network byte order

    /* Bind the socket with the server address now the socket know what he listen to*/
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if ((fp = fopen("log", "a+")) == NULL) {
        perror("failed to open log file");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on %s:%d\n", SERVER_IP, PORT);

    while (1) {
        len = sizeof(cliaddr);
        n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0, (struct sockaddr *)&cliaddr, &len);
        if(n <= 0){
            printf("bad recive from client\n");
            continue;
        }
        buffer[n] = '\0';

        if (strcmp(inet_ntoa(cliaddr.sin_addr), CLIENT_IP) == 0) { /*Check if the message is from the expected client IP*/
            if (strncmp(buffer,"read",4) == 0) {  
                printf("1\n");
                status = check_sendToUser(fp,sockfd ,cliaddr,len,buffer,msgs);
                if(status)
                    continue;
            }

            if(strncmp(buffer,"write",5) == 0){
                status = sendto(sockfd, (const char *)recvbuf, strlen(recvbuf), 0, (const struct sockaddr *)&cliaddr, len);
                printToLog(fp, buffer);
            }

            if (strncmp(buffer, "exit",4) == 0) {
                status = sendto(sockfd, (const char *)buffer, strlen(buffer), 0, (const struct sockaddr *)&cliaddr, len);
                if(status == -1)
                    printf("sendto failed\n");
                printf("Server closing\n");
                fclose(fp);
                break;
            }
            printf("Received log: %s\n", buffer);
        }
    }
    
    close(sockfd);
    return 0;
}

int check_sendToUser(FILE *fp, int sockfd, struct sockaddr_in cliaddr, int len, char buffer[MAXLINE],masseges msgs) {
    int lines = -1 , linesreq = 0;

    int status = sscanf(buffer, "read %d", &linesreq); //parse the message
    if (status != 1) {
        printf("sscanf fail\n");
        return 1;
    }
    if (linesreq <= 0 || linesreq > 8)
        return 1;

    pthread_mutex_lock(&readLock);
    fseek(fp, 0, SEEK_END); // Go to the end of file to the next request
    long pos = ftell(fp); //position of the file pointer 

    while (pos && lines < linesreq) {
        fseek(fp, --pos, SEEK_SET);  // Move the pointer position one character back
        char ch = fgetc(fp); // read the character
        if (ch == '\n') {
            lines++;
        }
        if (pos == 0) // If we reach the beginning of the file, exit the loop
            break;
    }

    msgs.lines = lines;
    
    fseek(fp, ++pos, SEEK_SET); // Move the file pointer back to the correct position
    for (int i = 0; i < lines; i++) {
        fgets(msgs.msg[i], MAXLINE, fp);
        printf("%s\n",msgs.msg[i]);
    }
    
    // Send the messages to the client
    fseek(fp, 0, SEEK_END); // Go to the end of file to the next request
    pthread_mutex_unlock(&readLock);

    status = sendto(sockfd, &msgs, sizeof(struct masseges), 0, (const struct sockaddr *)&cliaddr, len);

    return 0;
}

void printToLog(FILE *fp,char * buffer){
    char buf[MAXLINE];
    int status = sscanf(buffer,"write %s",buf); 
    if(status != 1){
        printf("to many letter or bad syntax\n"); //theck syntax and size both
        return;
    }
    pthread_mutex_lock(&writeLock);
    int chars_written = fprintf(fp, "%s", buffer + 6);
    pthread_mutex_unlock(&writeLock);
    if (chars_written < 0) {
        perror("fprintf failed");
    }
    else{
        fflush(fp);
    }
}

