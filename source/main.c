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
    if (sock < 0) return -1

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
    //implement
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

static void drawScreen(void){
    //implement function to print out the fetched data to the screens
}

static void setup_consoles(void){
    //implement
}

static bool wifi_connected(void){
    if (!Wifi_InitDefault(WFC_CONNECT)) return false;
    return true;
}

int main(void){
    setup_consoles(); //Initialize print consoles. To be implemented.
    consoleSelect(&topScreen);

    iprintf("Connecting to Wifi..\n");
    if (!wifi_connected()){
        iprintf("WiFi FAILED!\nCheck WFC settings, or restart and try again.\n");
        while (1) swiWaitForVBlank();
    }
    iprintf("WiFi OK!\n\nFetching Home Assistant data...\n");

    //fetch initial Information
    //implement fetch_entity and fetch_weather function

    //main loop
    while(1){
        //implement draw function for both Screens
        //implement manual and automatic refresh
    }
}