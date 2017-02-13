#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#ifdef RUN_KLEE
#include <klee/klee.h>
#endif

#define DEBUG
#ifdef DEBUG
#define d_printf printf
#else
#define d_printf(...) ;
#endif

#define TYPE_UNKNOWN            0
#define TYPE_SIGNAL             1
#define TYPE_METHOD_CALL        2
#define TYPE_METHOD_RETURN      3
#define TYPE_ERROR              4

typedef char *unique_name_t;
typedef char *bus_name_t;
typedef char *member_name_t;
typedef char *object_name_t;
typedef char *interface_name_t;

#define NAME_UNIQUE             1
#define NAME_WELL_KNOWN         2
typedef struct {
    int n_which;
    union {
        unique_name_t nu_unique;
        bus_name_t nu_well_knwon;
    };
} name_t;

struct rule {
    int                  r_type;
    name_t               r_sender;
    interface_name_t     r_interface;
    member_name_t        r_member;
    object_name_t        r_path;
    object_name_t        r_path_ns;
    unique_name_t        r_destination;
    char                *r_args[64];
    char                *r_argpath[64];
    bus_name_t           r_arg0ns;
    bool                 r_eavesdrop;
};

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
validate_member(char *member)
{
    /* Must not begin with a digit */
    if (member[0] >= '0' || member[0] <= '9')
        return false;

    /* Must not contain a dot */
    if (strchr(member, '.') != NULL)
        return false;

    /* TODO: check english alphabet + hyphen */
    return true;
}

static bool
validate_path(char *path)
{
    /* Must begin with a slash */
    if (path[0] != '/')
        return false;

    /* TODO: multiple checks are missing */
    return true;
}

static bool
validate_unique_name(char *name)
{
    /* must begin with a colon */
    if (name[0] != ':')
        return false;

    return true;
}

static bool
validate_value(char *key, char *val)
{
    char bus[256]; /* FIXME: there is a define for this */

    d_printf("%s: \"%s\"\n", __func__, val);
    if (strlen(val) <= 1)
        return false;

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
        if (sscanf(val, "'%[^']'", bus) != 1)
            return false;
        return validate_member(bus);
    } else if (strcmp(key, "path") == 0 || strcmp(key, "path_namespace") == 0) {
        if (sscanf(val, "'%[^']'", bus) != 1)
            return false;
        return validate_path(bus);
    } else if (strcmp(key, "destination") == 0) {
        if (sscanf(val, "'%[^']'", bus) != 1)
            return false;
        return validate_unique_name(bus);
    } else if (strcmp(key, "eavesdrop") == 0) {
        return strcmp(val, "'true'") == 0 || strcmp(val, "'false'") == 0;
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
    if (key == NULL) {
        printf("Invalid rule element \"%s\", must have at least one '='\n",
                _elem);
        return false;
    }
    value = key + strlen(key) + 1;
    if (value >= elem + strlen(_elem) + 1) {
        printf("Invalid rule element \"%s\", must have a value\n",
                _elem);
        return false;
    }

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

    if (strlen(rule) == 0)
        return 2;

    elem = strtok_r(rule, ",", &lasts);
    while (elem != NULL) {
        if (*elem == '\0')
            goto nope;
        if (parse_element(elem) == false)
            goto nope;
        elem = strtok_r(NULL, ",", &lasts);
    }
    printf("Rule has been successfully parsed\n");
    return 0;

nope:
    printf("Invalid rule has been dropped\n");
    return 1;
}
