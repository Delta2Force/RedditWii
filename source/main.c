// This is my first C script so it probably sucks.
// Over the past few weeks I have spent my time tinkering with this
// and I would have never thought that I would have to work with
// sockets like this. I'm even scared to do this in Java!
// I think this is my favourite project yet :)
//
// Written by u/DeltaTwoForce

#include <grrlib.h>

#include <stdlib.h>
#include <math.h>
#include <wiiuse/wpad.h>
#include <stdio.h>

#include "json.h"
#include "NotoSans_ttf.h"
#include "gfx/logo.h"
#include "gfx/logotext.h"
#include <string.h>

#include <unistd.h>
#include <network.h>

#define dolphin

struct post {
    char author[25];
    char subreddit[25];
    char title[300];
    char url[300];
    int score;
};

float lerp(float a, float b, float t);
void front_page(void);
enum state {LOADING_SCREEN, FRONT_PAGE};

unsigned int evctr = 0;

void countevs(int chan, const WPADData *data) {
    evctr++;
}

float redditTextVis = 0;
enum state currentState = LOADING_SCREEN;

unsigned int logoX = 79; unsigned int logoY = 165;
float logoScale = 1;
bool frontpageGotten=false; bool getFrontpage=false;
struct post fp_data[25];

int main() {
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
                if(!getFrontpage){
                    getFrontpage = true;
                    front_page();
                }
            }

            if(frontpageGotten && getFrontpage){
                currentState = FRONT_PAGE;
            }
        }else{
            int yy = 32;
            for(int i = 0; i<10; i++){
                GRRLIB_PrintfTTF(0, yy, tex_font, fp_data[i].title, 16, 0x00000000);
                yy+=16;
            }

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
    int portno = 80;
    char *host = "api.pushshift.io";

    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd, bytes, sent, total, message_size;
    char *message, response[4096*2];

    message_size=0;
        message_size+=strlen("GET /reddit/search/submission/?score=>1000&size=10&fields=author,subreddit,url,score,title HTTP/1.1\r\nHost: api.pushshift.io\r\n");
        message_size+=strlen("\r\n");

    message=malloc(message_size);
        sprintf(message,"GET /reddit/search/submission/?score=>1000&size=10&fields=author,subreddit,url,score,title HTTP/1.1\r\nHost: api.pushshift.io\r\n");
        strcat(message,"\r\n");

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

    net_recv(sockfd, &response, total, 0); //I spent a week tinkering with this code only for it to be fixed by replacing it with this one line. ONE. LINE.
    net_close(sockfd);

    free(message);

    char* rawjson = strstr(response, "\r\n\r\n");
    rawjson+=4;

    json_value* js = json_parse(rawjson, strlen(rawjson));
    json_value* data = js->u.object.values[0].value;

    int length, x;
    length = data->u.array.length;
    for(x = 0; x < length; x++){
        json_value* arrval = data->u.array.values[x];
        json_value* author = arrval->u.object.values[0].value;
        json_value* score = arrval->u.object.values[1].value;
        json_value* subreddit = arrval->u.object.values[2].value;
        json_value* title = arrval->u.object.values[3].value;
        json_value* url = arrval->u.object.values[4].value;

        struct post i;
        strcpy(i.author, author->u.string.ptr);
        i.score = score->u.integer;
        strcpy(i.subreddit, subreddit->u.string.ptr);
        strcpy(i.url, url->u.string.ptr);
        strcpy(i.title, title->u.string.ptr);
        fp_data[x] = i;
    }

    frontpageGotten = true;
}