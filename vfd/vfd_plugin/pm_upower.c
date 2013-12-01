/* Source: https://github.com/KABA-CCEAC/node-pm-notify/blob/master/src/pm_linux.cpp */

#include <dbus/dbus-glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
//#include <stdbool.h>
//#include "pm.h"
//#include "constants.h"

#ifndef PM_NOTIFY_TEST
#include "vfdm.h"
#include "sysmon.h"
#endif


/**********************************
 * Local defines
 **********************************/
#define RESUME_SIGNAL   "Resuming"
#define SLEEP_SIGNAL    "Sleeping"
#define POWER_INTERFACE "org.freedesktop.UPower"

#define RULE  "type='signal',interface='"POWER_INTERFACE"'"

/**********************************
* Local Variables
**********************************/

/**********************************
 * Local Helper Functions protoypes
 **********************************/

/**********************************
* Local Functions
***********************************/
static DBusHandlerResult signal_filter(DBusConnection *connection, DBusMessage *msg, void *user_data)
{
    if (dbus_message_is_signal(msg, POWER_INTERFACE, RESUME_SIGNAL))
    {
#ifdef PM_NOTIFY_TEST
        fprintf(stderr, "RESUME");
#else
        vfdm_pmwake();
#endif
    }
    else if (dbus_message_is_signal(msg, POWER_INTERFACE, SLEEP_SIGNAL))
    {
#ifdef PM_NOTIFY_TEST
        fprintf(stderr, "STANDBY");
#else
        vfdm_pmsuspend();
#endif
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


void pm_upower_init()
{
    DBusError error;

    dbus_error_init(&error);
    DBusConnection *conn = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

    if (dbus_error_is_set(&error))
    {
        g_error("Cannot get System BUS connection: %s", error.message);
        dbus_error_free(&error);
        return;
    }

    dbus_connection_setup_with_g_main(conn, NULL);

    dbus_bus_add_match(conn, RULE, &error);

    if (dbus_error_is_set(&error))
    {
        g_error("Cannot add D-BUS match rule, cause: %s", error.message);
        dbus_error_free(&error);
        return;
    }

    dbus_connection_add_filter(conn, signal_filter, NULL, NULL);

}

#ifdef PM_NOTIFY_TEST
int main(void)
{
    InitDbus();

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    g_main_loop_run(loop);

    while (1) sleep(1);
//    return 0;
}
#endif


