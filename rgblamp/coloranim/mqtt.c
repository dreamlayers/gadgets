#include <stdio.h>
#include <mosquitto.h>
#include "coloranim.h"

#define NAME "Behind TV"
#define OBJECT_ID "ws281x"

const char discovery[] = "{\
\"name\": \"" NAME "\", \
\"schema\": \"json\", \
\"command_topic\": \"homeassistant/light/" OBJECT_ID "/set\", \
\"state_topic\": \"homeassistant/light/" OBJECT_ID "/state\", \
\"brightness\": \"true\", \
\"rgb\": \"true\"\
}";

static void my_message_callback(struct mosquitto *mosq, void *userdata,
                                const struct mosquitto_message *message)
{
    if (message->payloadlen) {
#ifdef DEBUG
        printf("%s %s\n", message->topic, message->payload);
#endif
        if (json_parse(message->payload, message->payloadlen)) {
            mqtt_new_state();
        }
        /* TODO Maybe echoing everything back isn't best */
        mosquitto_publish(mosq, NULL,
                          "homeassistant/light/" OBJECT_ID "/state",
                          message->payloadlen, message->payload,
                          1, 1);

    } else {
        printf("%s (null)\n", message->topic);
    }
    fflush(stdout);
}

static void my_connect_callback(struct mosquitto *mosq, void *userdata,
                                int result)
{
    if (!result) {
        /* Subscribe to broker information topics on successful connect. */
        mosquitto_subscribe(mosq, NULL,
                            "homeassistant/light/" OBJECT_ID "/set", 2);
        /* Publish Home Assistant discovery message */
        mosquitto_publish(mosq, NULL,
                          "homeassistant/light/" OBJECT_ID "/config",
                          /* Don't send the null terminator! */
                          sizeof(discovery)-1, discovery,
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

int main(int argc, char *argv[])
{
    char *host = "localhost";
    int port = 1883;
    int keepalive = 60;
    bool clean_session = true;
    struct mosquitto *mosq = NULL;

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
    mosquitto_will_set(mosq, "homeassistant/light/" OBJECT_ID "/config",
                       0, NULL, 1, 1);

    if (mosquitto_connect(mosq, host, port, keepalive)) {
        fprintf(stderr, "Unable to connect.\n");
        return 1;
    }

    render_open();

    mosquitto_loop_forever(mosq, -1, 1);

    render_close();
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
