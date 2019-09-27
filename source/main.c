#include <grrlib.h>

#include <stdlib.h>
#include <math.h>
#include <wiiuse/wpad.h>
#include <stdio.h>

#include "jsmn.h"
#include "NotoSans_ttf.h"
#include "gfx/logo.h"
#include "gfx/logotext.h"
#include <string.h>

#include <unistd.h>
#include <network.h>
/*#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>*/

#define dolphin

// This is my first C script so it probably sucks.
// Written by u/DeltaTwoForce

float lerp(float a, float b, float t);
void front_page(void);
enum state {LOADING_SCREEN, FRONT_PAGE};

unsigned int evctr = 0;

void countevs(int chan, const WPADData *data) {
    evctr++;
}

float redditTextVis = 0;
enum state currentState = LOADING_SCREEN;
char* msg;

unsigned int logoX = 79; unsigned int logoY = 165;
float logoScale = 1;
bool frontpageGotten=false; bool getFrontpage=false;

int main() {
    jsmn_parser parser;
    jsmn_init(&parser);

    GRRLIB_Init();
    PAD_Init();
    WPAD_Init();
    net_init();

    GRRLIB_ttfFont *tex_font = GRRLIB_LoadTTF(NotoSans_ttf, NotoSans_ttf_size);
    GRRLIB_texImg *redditLogoText = GRRLIB_LoadTexture(logotext);
    GRRLIB_texImg *redditLogo = GRRLIB_LoadTexture(logo);

    GRRLIB_SetBackgroundColour(0xFF, 0xFF, 0xFF, 0xFF);

    WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
    WPAD_SetVRes(WPAD_CHAN_ALL, 640, 480);

    // 79, 165

    while(1) {
        GRRLIB_2dMode();
        WPAD_ReadPending(WPAD_CHAN_ALL, countevs);
        u32 down = WPAD_ButtonsDown(WPAD_CHAN_0);
        if(down & WPAD_BUTTON_HOME) break;

        unsigned int endsUp = redditTextVis*0xFF;
        GRRLIB_DrawImg(0, 0, redditLogoText, 0, 1, 1, 0xFFFFFF00|endsUp);

        if(currentState == LOADING_SCREEN){
            GRRLIB_DrawImg(logoX, logoY, redditLogo, 0, logoScale, logoScale, 0xFFFFFF00|endsUp);
            redditTextVis = lerp(redditTextVis,1,0.1);

            //When Reddit logo is fully faded in
            if (endsUp >= 0xF0){
                GRRLIB_PrintfTTF(4, 4, tex_font, "Written by u/DeltaTwoForce", 16, 0x00000000);
                GRRLIB_PrintfTTF((640/2) - sizeof("Getting front page...")*7, (480/3)*2, tex_font, "Getting front page...", 32, 0x00000000);
            }

            if (endsUp >= 0xFD){
                GRRLIB_PrintfTTF(4, 4, tex_font, "Written by u/DeltaTwoForce", 16, 0x00000000);
                if(!getFrontpage){
                    front_page();
                    getFrontpage = true;
                }
            }

            if(frontpageGotten && getFrontpage){
                currentState = FRONT_PAGE;
            }
        }else{
            GRRLIB_PrintfTTF(4, 4, tex_font, msg, 16, 0x00000000);
            GRRLIB_DrawImg(logoX, logoY, redditLogo, 0, logoScale, logoScale, 0xFFFFFFFF);
            redditTextVis = lerp(redditTextVis,0,0.1);
            logoX = lerp(logoX,4,0.1);
            logoY = lerp(logoY,4,0.1);
            logoScale = lerp(logoScale,0.4,0.1);
        }

        GRRLIB_Render();
    }
    GRRLIB_FreeTTF(tex_font);
    GRRLIB_FreeTexture(redditLogoText);
    GRRLIB_FreeTexture(redditLogo);
    GRRLIB_Exit(); // Be a good boy, clear the memory allocated by GRRLIB
    WPAD_Shutdown();

    exit(0);
}

float lerp(float a, float b, float t){
    return a+(b-a)*t;
}

void front_page(){
    int portno =        80;
    char *host =        "api.pushshift.io";
    char *message = "GET /reddit/search/submission/?score=>10000&size=10&fields=author,url,score,title HTTP/1.1\r\nHost: api.pushshift.io\r\nUser-Agent: RedditWii/1.0\r\n\r\n";

    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd, bytes, sent, received, total;

    char response[6000];

    //printf("Request:\n%s\n",message);

    sockfd = net_socket(AF_INET, SOCK_STREAM, 0);
    //if (sockfd < 0) printf("ERROR opening socket");

    struct timeval tv;
    tv.tv_sec = 30;  /* 30 Secs Timeout */
    net_setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval));

    server = net_gethostbyname(host);
    //if (server == NULL) printf("ERROR, no such host");

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

    if (net_connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){}
        //printf("ERROR connecting");

    total = strlen(message);
    sent = 0;
    do {
        bytes = net_write(sockfd,message+sent,total-sent);
        //if (bytes < 0)
            //printf("ERROR writing message to socket");
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);

    memset(response,0,sizeof(response));
    total = sizeof(response)-1;
    received = 0;
    do {
        bytes = net_read(sockfd,response+received,total-received);
        //if (bytes < 0)
            //printf("ERROR reading response from socket");
        if (bytes == 0)
            break;
        received+=bytes;
    } while (received < total);

    net_close(sockfd);

    msg = malloc(sizeof(response));
    sprintf(msg, "%s", response);

    FILE * fp;
    fp = fopen("sd://test.json","w");
    fprintf(fp, "%s %s :NICE\n", host, message);
    fprintf(fp, response);
    fclose(fp);

    frontpageGotten = true;
}

/*
    int portno =        80;
    char *host =        "api.pushshift.io";
    char *message = "GET /reddit/search/submission/?score=%%3E10000&size=10&fields=author,subreddit,url,score,title HTTP/1.1\r\nHost: api.pushshift.io\r\nUser-Agent: RedditWii/1.0\r\n\r\n";

    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd, bytes, sent, received, total;

    printf("Request:\n%s\n",message);

    sockfd = net_socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) printf("ERROR opening socket");

    server = net_gethostbyname(host);
    if (server == NULL) printf("ERROR, no such host");

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

    if (net_connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        printf("ERROR connecting");

    total = strlen(message);
    sent = 0;
    do {
        bytes = net_write(sockfd,message+sent,total-sent);
        if (bytes < 0)
            printf("ERROR writing message to socket");
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);

    memset(response,0,sizeof(response));
    total = sizeof(response)-1;
    received = 0;
    do {
        bytes = net_read(sockfd,response+received,total-received);
        if (bytes < 0)
            printf("ERROR reading response from socket");
        if (bytes == 0)
            break;
        received+=bytes;
    } while (received < total);

    if (received == total)
        printf("ERROR storing complete response from socket");

    net_close(sockfd);

    FILE * fp;
    fp = fopen("sd://test.json","w");
    fprintf(fp, "%s %s :NICE\n", host, message);
    fprintf(fp, response);
    fclose(fp);
}*/