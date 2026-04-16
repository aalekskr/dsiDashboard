#include <nds.h>
#include <dswifi9.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <sdtlib.h>
#include <string.h>
#include <time.h>

//The HomeAssistant config is currently baked into the code while compiling
//Make sure to change the 3 Variables according to your HA config
#define HA_HOST_IP "192.168.0.0"
#define HA_HOST_PORT 8123
#define HA_LLA_TOKEN "Your long lived access Token"

/*Make sure to define *your own* HA entities. 
You can have up to 8 Entities displayed*/
#define ENTITIES_NUM 5
static const char *ENTITIES_IDS[ENTITIES_NUM] = {
    "light.kajplats_e14_ws_globe_806lm",
    "switch.schreibtischlampe",
    "switch.leselampe",
    "sensor.eingangstuere",
    "sensor.wohnzimmer_temperature"
}

//For the weather entity to work, find out *your own* home weater Entity
#define WEATHER_ENTITY "weather.forecast_home"

#define SCREEN_X 256
#define SCREEN_Y 192
#define MAX_RESP 4096

typedef struct {
    char entity_id[64];
    char state[32];
    char entity_name[64];
} HAEntity;

static HAEntity entities[ENTITIES_NUM];
static char weather_state[32] = "n/a";
static char weather_temperature[16] = "n/a";

static PrintConsole topScreen;
static PrintConsole bottomScreen;

//HTTP GET function, returns bytes read. Writes into buf.
static int http_get(const char *host, int port, const char *path, const char *token, char *buf, int bufsz){
    struct hostent *he = gethostbyname(host);
    if (!he) return -1;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        closesocket(sock);
        return -1;
    }
    char req[512];
    snprintf(req, sizeof(req),
        "GET %s HTTP/1.0\r\n"
        "Host: %s:%d\r\n"
        "Authorization: Bearer %s\r\n"
        "Content-Type: application/json\r\n"
        "\r\n", path, host, port, token);
    send(sock, req, strlen(req), 0);

    int total = 0, n;
    while (total < bufsz - 1 && (n = recv(sock, buf + total, bufsz -1 - total, 0)) > 0){
        total += n;
    }
    buf[total] = '\0';
    closesocket(sock);
    return total;
}

static void json_get(const char *json, const char *key, char *out, int outsz){
    out[0] = '\0';
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *p = strstr(json, search)
    if (!p) return;
    p += strlen(search);
    while (*p == ' ' || *p == ':') p++;
    char end_char = ',';
    if (p* == '"'){
        p++;
        end_char = '"';
    }
    else if (*p == '{' || *p == '[') return;


    int i = 0;
    while (*p && *p != end_char && *p != '}' && *p != '\n' && i < outsz -1){
        out[i++] = *p++;
    }
    out[i] = '\0'
}

static void fetch_entity(int enID){
    static char resp[MAX_RESP];
    char path[128];
    snprintf(path, sizeof(path), "/api/states/%s", ENTITIES_IDS[enID]);
    strncpy(entities[enID].entity_id, ENTITIES_IDS[enID], 63);
    if (http_get(HA_HOST_IP, HA_HOST_PORT, path, HA_LLA_TOKEN, resp, MAX_RESP) < 0){
        strcpy(entities[enID].state, "ERR");
        strcpy(entities[enID].entity_name, ENTITIES_IDS[enID]);
        return;
    }
    char *body = strstr(resp, "\r\n\r\n");
    if (!body){
        strcpy(entities[enID].state, "ERR");
        return;
    }
    body +=4;
    json_get(body, "state", entities[enID].state, 32);
    char *attrs = strstr(body, "\"attributes\"");
    if(attrs){
        json_get(attrs, "entity_name", entities[enID].entity_name, 64);
    }
    if (entities[enID].entity_name[0] == '\0'){
        strncpy(entities[enID].entity_name, ENTITIES_IDS[enID], 63);
    }

}

static void fetch_weather(void){
    static char resp[MAX_RESP];
    char path[128];
    snprintf(path, sizeof(path), "/api/states/%s", WEATHER_ENTITY);
    if (http_get(HA_HOST_IP, HA_HOST_PORT, path, HA_LLA_TOKEN, resp, MAX_RESP) < 0){
        strcpy(weather_state, "offline");
        return;
    }
    char *body = strstr(resp, "\r\n\r\n");
    if (!body) return;
    body += 4;
    json_get(body, "state", weather_state, 32);
    char *attrs = strstr(body, "\"attributes\"");
    if (attrs){
        json_get(attrs, "temperature", weather_temperature, 16);
    }
}

static void drawTopScreen(void){
    consoleClear();

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    iprintf("\x1b[0;0H");
    iprintf("*********************************\n");
    iprintf(" DSi Home Assistant Dashboard\n")
    iprintf("*********************************\n");

    iprintf("\n   Time : %02d:%02d:%02d\n", t->tm_hour, t->tm_min, t->tm_sec);
    iprintf("   Date : %02d.%02d.%04d", t->tm_mday, t->tm_mon, t->tm_year);

    iprintf("\n   Weather : %s\n", weather_state);
    iprintf("   Temp   : %s °C\n", weather_temperature);

    iprintf("\n");
    iprintf("*********************************\n");
    iprintf("    [START] Refresh all data\n")
    iprintf("*********************************\n");
}

static void drawBottomScreen(void){
    consoleSelect(&bottomScreen);
    consoleClear();

    iprintf("\x1b[0;0H")
    iprintf("*********************************\n");
    iprintf(" Lights & Sensors\n");
    iprintf("*********************************\n");

    for (int i = 0; i < ENTITIES_NUM; i++){
        const char *icon = "[ ]";
        if (strcmp(entities[i].state, "on") == 0) icon = "[x]";
        else if (strcmp(entities[i].state, "off") == 0) icon = "[ ]";
        else icon = "[i]"; //Unknown switch/sensor

        char name[22];
        strcpy(name, entities[i].friendly_name, 21);
        name[21] = '\0';

        iprintf(" %s %-21s\n", icon, name);
        iprintf("     State: %s\n\n", entities[i].state);
    }
    iprintf("*********************************\n");
    iprintf("         [A] Refresh\n");
}

static void setup_consoles(void){
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);
    consoleInit(&topScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
    consoleInit(&bottomScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
}

static bool wifi_connected(void){
    if (!Wifi_InitDefault(WFC_CONNECT)) return false;
    return true;
}

int main(void){
    setup_consoles();
    consoleSelect(&topScreen);

    iprintf("Connecting to Wifi..\n");
    if (!wifi_connected()){
        iprintf("WiFi FAILED!\nCheck WFC settings.\nOr restart and try again.\n");
        while (1) swiWaitForVBlank();
    }
    iprintf("WiFi OK!\n\nFetching Home Assistant data...\n");

    fetch_weather();
    for (int i = 0; i < ENTITIES_NUM; i++){
        fetch_entity(i);
    }

    u32 refresh_timer = 0;
    const u32 AUTO_REFRESH_FRAMES = 60 * 30; 

        //main loop
    while(1){
        swiWaitForVBlank();
        scanKeys();
        u32 keys = keysDown();

        if (keys & KEY_START || keys & KEY_A){
            consoleSelect(&topScreen);
            consoleClear();
            iprintf("Refreshing data...\n");
            fetch_weather();
            for (int i = 0; i < ENTITIES_NUM; i++){
                fetch_entity(i);
            }
            refresh_timer = 0;
        }

        refresh_timer++;
        if (refresh_timer >= AUTO_REFRESH_FRAMES){
            fetch_weather;
            for (int i = 0; i < ENTITIES_NUM; i++){
                fetch_entity(i);
            }
            refresh_timer = 0;
        }

        consoleSelect(&topScreen);
        drawScreen();
        consoleSelect(&topScreen);
        
    }
    return 0;
}