#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#ifdef WIN32
#define LIBMOSQUITTO_STATIC
#endif
#include <mosquitto.h>
#include "coloranim.h"

#ifndef MQTT_NAME
#define MQTT_NAME "Behind TV"
#endif
#ifndef MQTT_ID
#define MQTT_ID "ws281x"
#endif

static const char discovery_1[] = "{\
\"name\": \"" MQTT_NAME "\", \
\"schema\": \"json\", \
\"command_topic\": \"homeassistant/light/" MQTT_ID "/set\", \
\"state_topic\": \"homeassistant/light/" MQTT_ID "/state\", \
\"brightness\": \"true\", \
\"rgb\": \"true\", \
\"effect\": \"true\", \
\"effect_list\": ";

static const char discovery_3[] = " }";

static const char *discovery = NULL;
int discovery_len = 0;

static void build_discovery(void)
{
    char *d, *p;
    int l_1, l_2, l_3;

    l_1 = strlen(discovery_1);
    l_2 = effect_list_len();
    l_3 = strlen(discovery_3);
    d = malloc(l_1 + l_2 + l_3 + 1);
    if (d == NULL) fatal("Out of memory.");

    memcpy(d, discovery_1, l_1);
    p = d + l_1;
    effect_list_fill(p);
    p += l_2;
    memcpy(p, discovery_3, l_3);
    p += l_3;
    *p = 0;
    discovery_len = p - d;
    discovery = d;
}

/* MQTT coloranim */
#define MQTT_SCALE 255
static void mqtt_new_state(void)
{
    //static char col[3][20];
    //static char *cmd[] = { "color", col[0], col[1], col[2] };

    static int state = 0;
    static int rgb[3] = { MQTT_SCALE, MQTT_SCALE, MQTT_SCALE };
    static int brightness = MQTT_SCALE;
    static char effect[40];

    get_state(&state);
    get_brightness(&brightness);
    get_color(rgb);
    get_effect(effect, sizeof(effect));

    if (state) {
        if (effect[0] == 0) {
            static char buf[100];
            int l;
            l = snprintf(buf, sizeof(buf), "%f %f %f",
                         rgb[0] * brightness / (MQTT_SCALE * MQTT_SCALE * 1.0),
                         rgb[1] * brightness / (MQTT_SCALE * MQTT_SCALE * 1.0),
                         rgb[2] * brightness / (MQTT_SCALE * MQTT_SCALE * 1.0));
            cmd_enq_string(2, buf, l);
        } else {
            cmd_enq_string(3, effect, strlen(effect));
        }
    } else {
        cmd_enq_string(2, "0", 1);
    }
}

static void my_message_callback(struct mosquitto *mosq, void *userdata,
                                const struct mosquitto_message *message)
{
    if (message->payloadlen) {
#ifdef DEBUG
        printf("%s %s\n", message->topic, message->payload);
        fflush(stdout);
#endif
        if (json_parse(message->payload, message->payloadlen)) {
            mqtt_new_state();
        }
    }
#ifdef DEBUG
    else {
        printf("%s (null)\n", message->topic);
        fflush(stdout);
    }
#endif
}

static void my_connect_callback(struct mosquitto *mosq, void *userdata,
                                int result)
{
    if (!result) {
        /* Subscribe to broker information topics on successful connect. */
        mosquitto_subscribe(mosq, NULL,
                            "homeassistant/light/" MQTT_ID "/set", 2);
        /* Publish Home Assistant discovery message */
        if (discovery == NULL) {
            build_discovery();
        }
        mosquitto_publish(mosq, NULL,
                          "homeassistant/light/" MQTT_ID "/config",
                          /* Don't send the null terminator! */
                          discovery_len, discovery,
                          1, 1);
    } else {
        fprintf(stderr, "Connect failed\n");
    }
}

#ifdef DEBUG
static void my_subscribe_callback(struct mosquitto *mosq, void *userdata,
                                  int mid, int qos_count,
                                  const int *granted_qos)
{
    int i;

    printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
    for (i=1; i<qos_count; i++) {
        printf(", %d", granted_qos[i]);
    }
    printf("\n");
}

static void my_log_callback(struct mosquitto *mosq, void *userdata,
                            int level, const char *str)
{
    /* Pring all log messages regardless of level. */
    printf("%s\n", str);
}
#endif

static struct mosquitto *mosq = NULL;

int mqtt_init(void)
{
    char *host = "192.168.1.24";
    int port = 1883;
    int keepalive = 60;
    bool clean_session = true;

    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, clean_session, NULL);
    if (!mosq) {
        fprintf(stderr, "Error: Out of memory.\n");
        return 1;
    }
#ifdef DEBUG
    mosquitto_log_callback_set(mosq, my_log_callback);
    mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
#endif
    mosquitto_connect_callback_set(mosq, my_connect_callback);
    mosquitto_message_callback_set(mosq, my_message_callback);
    mosquitto_will_set(mosq, "homeassistant/light/" MQTT_ID "/config",
                       0, NULL, 1, 1);

    if (mosquitto_connect(mosq, host, port, keepalive)) {
        fprintf(stderr, "Unable to connect.\n");
        return 1;
    }

    return mosquitto_loop_start(mosq);
}

void mqtt_quit()
{
    mosquitto_disconnect(mosq);
    mosquitto_loop_stop(mosq, false);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}

void mqtt_report_solid(const pixel pix)
{
    static const char template_on[] = "{\"state\": \"ON\", \"brightness\": %li, \"color\":{\"r\": %i, \"g\": %i, \"b\": %i}, \"effect\": \"\"}";
    static const char message_off[] = "{\"state\": \"OFF\" }";
    int i, rgb[COLORCNT];
    double max = 0;

    for (i = 0; i < COLORCNT; i++) {
        if (max < pix[i]) max = pix[i];
    }

    if (max == 0) {
        mosquitto_publish(mosq, NULL,
                          "homeassistant/light/" MQTT_ID "/state",
                          sizeof(message_off) - 1, message_off, 1, 1);

    } else {
        char buf[255];
        int len;
        /* Home Assistant wants colour and brightness separately */
        for (i = 0; i < COLORCNT; i++) {
            rgb[i] = lrint(pix[i] * MQTT_SCALE / max);
        }
        len = snprintf(buf, sizeof(buf), template_on,
                       lrint(max * MQTT_SCALE), rgb[0], rgb[1], rgb[2]);
        mosquitto_publish(mosq, NULL,
                          "homeassistant/light/" MQTT_ID "/state",
                          len, buf, 1, 1);

    }
}

void mqtt_report_effect(const char *name)
{
    static const char template[] = "{\"state\": \"ON\", \"brightness\": 255, \"effect\": \"%s\"}";
    char buf[255];
    int len;

    len = snprintf(buf, sizeof(buf), template, name);

    mosquitto_publish(mosq, NULL,
                      "homeassistant/light/" MQTT_ID "/state",
                      len, buf, 1, 1);
}
