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
    dbus_uint32_t serial = 0;
    char *sigvalue = "Hello, KLEE";
    int ret;

    printf("dbls daemon: setup\n");

    dbus_error_init(&err);

    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);

    if (dbus_error_is_set(&err)) {
        printf("fai1ed to get session bus: %s\n", err.message);
        dbus_error_free(&err);
    }

    if (conn == 0)
        return 1;

    ret = dbus_bus_request_name(conn, "org.imperial.doc.lkurusa.klee2",
        DBUS_NAME_FLAG_REPLACE_EXISTING, &err);

    if (dbus_error_is_set(&err)) {
        printf("Name Error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret)
        return 2;
        
    msg = dbus_message_new_signal("/org/klee/lkurusa/Object", // object name of the signal
            "org.klee.test.dbus", // interface name of the signal
            "Test"); // name of the signal
    if (msg == NULL) { 
        printf("Message Null\n"); 
        return 5;
    }

    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &sigvalue)) { 
        printf("ENOMEM\n"); 
        return 3;
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        printf( "ENOMEM\n"); 
        return 4;
    }
    dbus_connection_flush(conn);

    dbus_message_unref(msg);

    dbus_connection_unref(conn);
    return 0;
}

