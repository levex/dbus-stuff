#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define DEBUG
#ifdef DEBUG
#define d_printf printf
#else
#define d_printf(...) ;
#endif

static bool
test_arg(char *key)
{
    int n;

    if (sscanf(key, "arg%d", &n) != 1)
        return false;

    return n < 64;
}

static bool
test_argpath(char *key)
{
    int n;

    if (sscanf(key, "arg%dpath", &n) != 1)
        return false;

    return n < 64;
}

static bool
validate_key(char *key)
{
    return
        strcmp(key, "type") == 0 ||
        strcmp(key, "sender") == 0 ||
        strcmp(key, "interface") == 0 ||
        strcmp(key, "member") == 0 ||
        strcmp(key, "path") == 0 ||
        strcmp(key, "path_namespace") == 0 ||
        strcmp(key, "destination") == 0 ||
        strcmp(key, "eavesdrop") == 0 ||
        test_arg(key) ||
        test_argpath(key);
}

static bool
validate_bus_name(char *bus)
{
    d_printf("%s: \"%s\"\n", __func__, bus);

    /* This is a Unique Connection Name */
    if (bus[0] == ':')
        return true;

    /* A Name must have two elements and at least one dot */
    if (strchr(bus, '.') == NULL)
        return false;

    /* A Name must not begin with a dot */
    if (bus[0] == '.')
        return false;

    /* TODO: check length, check English alphabet + {_-} only */

    return true;
}

static bool
validate_interface_name(char *bus)
{
    if (validate_bus_name(bus) == false)
        return false;

    /* an interface name must not contain a hyphen */
    return strchr(bus, '-') == NULL;
}

static bool
validate_value(char *key, char *val)
{
    char bus[256]; /* FIXME: there is a define for this */

    if (strcmp(key, "type") == 0) {
        return strcmp(val, "'signal'") == 0
            || strcmp(val, "'method_call'") == 0
            || strcmp(val, "'method_return'") == 0
            || strcmp(val, "'error'") == 0;
    } else if (strcmp(key, "sender") == 0) {
        if (sscanf(val, "'%[^']'", bus) != 1)
            return false;
        return validate_bus_name(bus);
    } else if (strcmp(key, "interface") == 0) {
        if (sscanf(val, "'%[^']'", bus) != 1)
            return false;
        return validate_interface_name(bus);

    } else if (strcmp(key, "member") == 0) {
        return true;
    } else if (strcmp(key, "path") == 0 || strcmp(key, "path_namespace") == 0) {
        return true;
    } else if (strcmp(key, "destination") == 0) {
        return true;
    }
    /* TODO: fix the cases and add remaining ones */
    
    return false;
}

static bool
parse_element(char *_elem)
{
    char *elem, *key, *value, *lasts;

    elem = strdup(_elem);
    d_printf("%s: \"%s\"\n", __func__, elem);

    key = strtok_r(elem, "=", &lasts);
    value = key + strlen(key) + 1;

    d_printf("%s: KEY=\"%s\" VALUE=\"%s\"\n", __func__, key, value);
    if (!validate_key(key)) {
        printf("Invalid key (%s) supplied, rejecting rule\n", key);
        return false;
    }
    if (!validate_value(key, value)) {
        printf("Invalid value (\"%s\") for key %s, rejecting rule\n",
                value, key);
        return false;
    }

    free(elem);
    return true;
}

int
main(int argc, char **argv)
{
    char *_rule, *rule, *elem, *lasts;

    if (argc != 2) {
        printf("USAGE: %s RULE\n", argv[0]);
        return 1;
    }

    _rule = argv[1];
    rule = strdup(_rule);

    elem = strtok_r(rule, ",", &lasts);
    while (elem != NULL) {
        if (parse_element(elem) == false)
            return 1;
        elem = strtok_r(NULL, ",", &lasts);
    }
}
