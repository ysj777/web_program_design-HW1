
#include <ctype.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

char TCP_head[] = "HTTP/1.1 200 OK\r\n";
char TCP_head_jpg[] = "HTTP/1.1 200 OK\r\n"
                            "Content-type: image/jepg\r\n";
char HTML1[] = "\r\n"
                            "<!DOCTYPE html>\r\n"
                            "<html><head><title>web_server</title>\r\n"
                            "</head>\r\n"
                            "<body>\r\n";
char select_image[] = "<form action=\"\" method=\"post\" enctype=\"multipart/form-data\">\r\n"
                        "Please select a image<br>\r\n"
                        "<input type=\"file\" name=\"img\">\r\n"
                        "<button type=\"submit\">submit</botton>\r\n"
                        "</form>\r\n";
char text[] = "<img src=\"/img.jpeg\" alt\"No img\">\r\n";
char HTML2[] = "</body></html>\r\n";

char buff[100000];
char picture[100000];

void strcat_pack_selc(char *pack) {
	pack[0] = '\0';
	strcat(pack, TCP_head);
	strcat(pack, HTML1);
	strcat(pack, select_image);
	strcat(pack, text);
	strcat(pack, HTML2);
}

char *get_picture(char *package_head) {
	char *c = package_head;
	c = strstr(c, "\r\n\r\n");
	c = c + 5;
	c = strstr(c, "\r\n\r\n");
	c = c + 4;
	return c;
}

int get_len(char *pack) {
	char *line = strstr(pack, "Content-Length: ");
	char n[10];
	int idx = 0;
	while(*(++line) != '\n') {
		if(isdigit(*line)) n[idx++] = *line;
	}
	n[idx] = 0;
	return atoi(n);
}
void picture_pack(char *pack) {
	pack[0] = '\0';
	strcat(pack, TCP_head_jpg);
	strcat(pack, (char *)picture);
}

void show_pack_msg(char *pack) {
    if(strncmp(buff, "GET ", 4) == 0 && strstr(buff, "GET /img.jpeg")) printf("Browser request Image\n");
    else if(strncmp(buff, "GET ", 4) == 0 && strstr(buff, "Accept: text/html")) printf("Browser request Index\n");
    else if(strncmp(buff, "POST ", 5) == 0 && strstr(buff, "multipart/form-data")) printf("Browser send a file\n");
	else printf("Browser send a packet\n");
}

int sock0; 
int sock_client;

int main(){  

    struct sockaddr_in addr;  
    struct sockaddr_in client;  
    //socklen_t len;   
    char pack[4096];

    /* 製作 socket */  
    sock0 = socket(AF_INET, SOCK_STREAM, 0);  
      
    /* 設定 socket */
    bzero(&addr, sizeof(addr));  
    addr.sin_family = AF_INET;  
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(5050); 
    bind(sock0, (struct sockaddr*)&addr, sizeof(addr));  
    printf("\t[Info] binding...\n");  
      
    /* 設定成等待來自 TCP 用戶端的連線要求狀態 */  
    listen(sock0, 1024);  
    printf("\t[Info] listening...\n");  
      
    /* 接受來自 TCP 用戶端地連線要求*/  
    printf("\t[Info] wait for connection...\n"); 
    while(1){
        //len = sizeof(client);  
        sock_client = accept(sock0, (struct sockaddr *)NULL, NULL);  
        printf("\t[Info] Testing...\n");  

        char *paddr_str = inet_ntoa(client.sin_addr);  
        printf("\t[Info] Receive connection from %s...\n", paddr_str);

        pid_t pid = fork();
        
        if(pid == -1){
			printf("fork error\n");
			exit(1);
		}

        if(!pid){
            if(read(sock_client,buff,100000) == -1){
                printf("read error\n");
                exit(1);
            }
            printf("%s\n",buff);
            pack[0]='\0';

            show_pack_msg(buff);

            if(strncmp(buff, "GET ", 4) == 0 && strstr(buff, "GET /img.jpeg")) {
				
				FILE *image = fopen("img.jpeg", "rb");
				if(image == NULL) {
					printf("fopen error\n");
					exit(1);
				}

				int image_fd = fileno(image);

				struct stat statbuf;
				fstat(image_fd, &statbuf);

				strcat(pack, TCP_head_jpg);
				sprintf(buff, "Content-Length: %d\r\n", (int) statbuf.st_size);
				strcat(pack, buff);
				strcat(pack, "\r\n");
				
				if(write(sock_client, pack, strlen(pack)) == -1) {
					printf("write error\n");
					exit(1);
				}

				sendfile(sock_client, image_fd, 0, statbuf.st_size);
				pack[0] = '\0';
			}

            else if(strncmp(buff, "GET ", 4) == 0 && strstr(buff, "Accept: text/html")) {
				strcat_pack_selc(pack);
				if(write(sock_client, pack, strlen(pack)) == -1) {
					printf("write error\n");
					exit(1);
				}
            }
            else if(strncmp(buff, "POST ", 5) == 0 && strstr(buff, "multipart/form-data")) {
				char *start = get_picture(buff);
				int len = get_len(buff);
				FILE *fp = fopen("img.jpeg", "w");
				if(fp == NULL) {
					printf("fopen error\n");
					exit(1);
				}
				printf("\n%d\n", len);
				fwrite(start, (sizeof(char) * len), 1, fp);				
				strcat_pack_selc(pack);
				if(write(sock_client, pack, strlen(pack)) == -1) {
					printf("write error\n");
					exit(1);
				}
            }
            else {
				strcat_pack_selc(pack);
				if(write(sock_client, pack, strlen(pack)) == -1) {
					printf("write error\n");
					exit(1);
				}
            }

            pack[0] = '\0';
			buff[0] = '\0';
			if(close(sock_client) == -1) {
				printf("close error\n");
				exit(1);
            }
            exit(0);
        }
        else {
			if(close(sock_client) == -1) {
				printf("close error\n");
				exit(1);
			}
		}
      }
    /* 傳送 5 個字元   
    printf("\t[Info] Say hello back...\n")    ;  
    write(sock_client, "HELLO\n", 6);  
      
    結束 TCP 對話  
    printf("\t[Info] Close client connection...\n");  
    close(sock_client);  
      
    結束 listen 的 socket  
    printf("\t[Info] Close self connection...\n");  
    close(sock0);  
    return 0;  
    */
}  
