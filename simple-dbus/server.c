// DBus "Client" example
// Copyright 2017 (c) Levente Kurusa <lkurusa@acm.org>

#include <dbus/dbus.h>
#include <stdio.h>

int
main(int argc, char *argv[])
{
    DBusError err;
    DBusConnection *conn;
    DBusMessage* msg;
    DBusMessageIter args;
    char *sigvalue;
    int ret;

    printf("dbus daemon: setup\n");

    dbus_error_init(&err);

    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);

    if (dbus_error_is_set(&err)) {
        printf("failed to get session bus: %s\n", err.message);
        dbus_error_free(&err);
    }

    if (conn == 0)
        return 1;

    ret = dbus_bus_request_name(conn, "org.imperial.doc.lkurusa.klee",
        DBUS_NAME_FLAG_REPLACE_EXISTING, &err);

    if (dbus_error_is_set(&err)) {
        printf("Name Error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret)
        return 2;

    dbus_bus_add_match(conn, 
        "type='signal',interface='org.klee.test.dbus'", &err);

    dbus_connection_flush(conn);
    if (dbus_error_is_set(&err)) { 
        printf("Match Error (%s)\n", err.message);
        return 3;
    }

    while (1) {
        dbus_connection_read_write(conn, 0);
        msg = dbus_connection_pop_message(conn);

        if (msg == NULL) { 
            sleep(1);
            continue;
        }

        if (dbus_message_is_signal(msg, "org.klee.test.dbus", "Test")) {
            if (!dbus_message_iter_init(msg, &args))
                printf("Message has no arguments!\n"); 
            else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) 
                printf("Argument is not string!\n"); 
            else {
                dbus_message_iter_get_basic(&args, &sigvalue);
                printf("Got Signal with value \"%s\"\n", sigvalue);
            }
        }

        // free the message
        dbus_message_unref(msg);
    }

    /* conn is shared, so don't do a close, but simply, unref it */
    dbus_connection_unref(conn);
    return 0;
}

