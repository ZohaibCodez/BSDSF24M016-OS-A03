#include "shell.h"

// Shell variables are stored in a linked list of (name, value) pairs.
typedef struct Var {
    char*       name;
    char*       value;
    struct Var* next;
} Var;

static Var* var_list = NULL;        // head of the list (NULL = no variables yet)

// A variable name starts with a letter or '_', then letters, digits or '_'.
static int is_name_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
static int is_name_char(char c) {
    return is_name_start(c) || (c >= '0' && c <= '9');
}

// Create the variable, or change its value if it already exists.
void set_variable(const char* name, const char* value) {
    for (Var* v = var_list; v != NULL; v = v->next) {
        if (strcmp(v->name, name) == 0) {       // found: replace the old value
            free(v->value);
            v->value = strdup(value);
            return;
        }
    }

    Var* node = malloc(sizeof(Var));            // not found: add a new node at the END
    node->name  = strdup(name);
    node->value = strdup(value);
    node->next  = NULL;

    if (var_list == NULL) {
        var_list = node;
    } else {
        Var* last = var_list;
        while (last->next != NULL) last = last->next;
        last->next = node;
    }
}

// Return the value, or NULL if there is no such variable.
const char* get_variable(const char* name) {
    for (Var* v = var_list; v != NULL; v = v->next) {
        if (strcmp(v->name, name) == 0) return v->value;
    }
    return NULL;
}

// The 'set' built-in: print every variable as NAME=value.
void print_variables(void) {
    for (Var* v = var_list; v != NULL; v = v->next)
        printf("%s=%s\n", v->name, v->value);
}

// Is this word an assignment like  NAME=value ?  (valid name, then '=')
int is_assignment(const char* token) {
    if (!is_name_start(token[0])) return 0;
    int i = 1;
    while (is_name_char(token[i])) i++;
    return token[i] == '=';
}

// Split "NAME=value" at the first '=' and store it.
void handle_assignment(const char* token) {
    const char* eq = strchr(token, '=');
    char* name = strndup(token, eq - token);    // the part before '='
    set_variable(name, eq + 1);                 // the part after '='
    free(name);
}

// Make a NEW string where every $NAME is replaced by its value.
// Order: shell variable first, then environment variable, else empty.
static char* expand_one(const char* s) {
    size_t cap = strlen(s) + 1;                 // grows when a value is inserted
    char*  out = malloc(cap);
    size_t len = 0;

    while (*s != '\0') {
        if (*s == '$' && is_name_start(s[1])) {
            const char* start = s + 1;          // read the name after '$'
            const char* end = start;
            while (is_name_char(*end)) end++;

            char* name = strndup(start, end - start);
            const char* value = get_variable(name);
            if (value == NULL) value = getenv(name);
            if (value == NULL) value = "";
            free(name);

            size_t vlen = strlen(value);
            cap += vlen;                        // make room for the value
            out = realloc(out, cap);
            memcpy(out + len, value, vlen);
            len += vlen;
            s = end;                            // continue after the name
        } else {
            out[len++] = *s++;                  // normal character: copy it
        }
    }
    out[len] = '\0';
    return out;
}

// Go through all words; replace each one that contains '$'.
void expand_variables(char** arglist) {
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strchr(arglist[i], '$') == NULL) continue;

        char* expanded = expand_one(arglist[i]);
        free(arglist[i]);                       // free the old word
        arglist[i] = expanded;                  // main() will free the new one later
    }
}
