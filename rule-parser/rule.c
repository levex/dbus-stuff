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

typedef struct __rule__INTERNAL {
    int                  r_type;
    name_t              *r_sender;
    interface_name_t     r_interface;
    member_name_t        r_member;
    object_name_t        r_path;
    object_name_t        r_path_ns;
    unique_name_t        r_destination;
    char                *r_args[64];
    char                *r_argpath[64];
    bus_name_t           r_arg0ns;
    bool                 r_eavesdrop;
} rule_t;

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

static void
rule_append(rule_t *rule, char *key, char *val)
{
    char bus[256];
    int c = sscanf(val, "'%[^']'", bus);

    if (strcmp(key, "type") == 0) {
        if (strcmp(val, "'signal'") == 0)
            rule->r_type = TYPE_SIGNAL;
        else if (strcmp(val, "'method_call'") == 0)
            rule->r_type = TYPE_METHOD_CALL;
        else if (strcmp(val, "'method_return'") == 0)
            rule->r_type = TYPE_METHOD_RETURN;
        else if (strcmp(val, "'error'") == 0)
            rule->r_type = TYPE_ERROR;
    } else if (strcmp(key, "sender") == 0) {
        name_t *name = malloc(sizeof(*name));
        if (name == NULL)
            return;
        if (validate_unique_name(bus)) {
            name->n_which = NAME_UNIQUE;
            name->nu_unique = strdup(bus);
        } else {
            name->n_which = NAME_WELL_KNOWN;
            name->nu_well_knwon = strdup(bus);
        }
    } else if (strcmp(key, "interface") == 0) {
        rule->r_interface = strdup(bus);
    } else if (strcmp(key, "member") == 0) {
        rule->r_member = strdup(bus);
    } else if (strcmp(key, "path") == 0) {
        rule->r_path = strdup(bus);
    } else if (strcmp(key, "path_namespace") == 0) {
        rule->r_path_ns = strdup(bus);
    } else if (strcmp(key, "destination") == 0) {
        rule->r_destination = strdup(bus);
    } else if (strcmp(key, "eavesdrop") == 0) {
        if (strcmp(bus, "true") == 0)
            rule->r_eavesdrop = true;
        else rule->r_eavesdrop = false;
    }
}

static bool
parse_element(rule_t *ret, char *_elem)
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

    rule_append(ret, key, value);

    free(elem);
    return true;
}

rule_t *
parse_rule(char *rule)
{
    char *lasts, *elem;
    rule_t *ret;

    if (strlen(rule) == 0)
        return NULL;

    ret = calloc(1, sizeof(*ret));
    if (ret == NULL)
        return NULL;

    elem = strtok_r(rule, ",", &lasts);
    while (elem != NULL) {
        if (*elem == '\0')
            goto nope;
        if (parse_element(ret, elem) == false)
            goto nope;
        elem = strtok_r(NULL, ",", &lasts);
    }
    return ret;
nope:
    free(ret);
    return NULL;
}

void
dump_rule(rule_t *r)
{
    printf("Dumping rule at 0x%p:\n", r);
    printf("  type: %s\n", r->r_type == TYPE_SIGNAL ? "signal" :
                            r->r_type == TYPE_METHOD_CALL ? "method_call" :
                            r->r_type == TYPE_METHOD_RETURN ? "method_return" :
                            r->r_type == TYPE_ERROR ? "error" : "unknown");
    if (r->r_sender && r->r_sender->n_which == NAME_UNIQUE)
        printf("  unique sender: %s\n", r->r_sender->nu_unique);
    else if (r->r_sender)
        printf("  well known sender: %s\n", r->r_sender->nu_well_knwon);

    if (r->r_interface)
        printf("  interface: %s\n", r->r_interface);

    if (r->r_member)
        printf("  member: %s\n", r->r_member);

    if (r->r_path)
        printf("  path: %s\n", r->r_path);

    if (r->r_path_ns)
        printf("  path namespace: %s\n", r->r_path_ns);

    if (r->r_destination)
        printf("  unique destination: %s\n", r->r_destination);

    if (r->r_arg0ns)
        printf("  arg 0 namespace: %s\n", r->r_arg0ns);

    printf("  eavesdrop: %s\n", r->r_eavesdrop ? "true" : "false");
}

void
free_rule(rule_t *r)
{
    free(r->r_sender->nu_unique);
    free(r->r_sender);
    free(r->r_arg0ns);
    free(r->r_destination);
    free(r->r_interface);
    free(r->r_member);
    free(r->r_path);
    free(r->r_path_ns);
    /* TODO: free args */
}

int
main(int argc, char **argv)
{
    char *_rule, *rule, *elem, *lasts;
    rule_t *s_rule;

    if (argc != 2) {
        printf("USAGE: %s RULE\n", argv[0]);
        return 1;
    }

    _rule = argv[1];
    rule = strdup(_rule);

    s_rule = parse_rule(rule);

    free(rule);
    if (s_rule) {
        printf("Rule has been successfully parsed\n");
        dump_rule(s_rule);
        return 0;
    } else {
        printf("Invalid rule has been dropped\n");
        return -1;
    }
}
