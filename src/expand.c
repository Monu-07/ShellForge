#define _POSIX_C_SOURCE 200809L
#include "expand.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *expand_variables(const char *word) {
    if (!word) return NULL;
    
    size_t cap = 256;
    size_t len = 0;
    char *result = malloc(cap);
    if (!result) return NULL;
    result[0] = '\0';
    
    size_t i = 0;
    while (word[i] != '\0') {
        if (word[i] == '$') {
            i++;
            if (word[i] == '\0') {
                if (len + 2 >= cap) {
                    cap *= 2;
                    char *tmp = realloc(result, cap);
                    if (!tmp) { free(result); return NULL; }
                    result = tmp;
                }
                result[len++] = '$';
                result[len] = '\0';
                break;
            }
            
            size_t start = i;
            while (word[i] != '\0' && (isalnum((unsigned char)word[i]) || word[i] == '_')) {
                i++;
            }
            size_t var_len = i - start;
            if (var_len == 0) {
                if (len + 2 >= cap) {
                    cap *= 2;
                    char *tmp = realloc(result, cap);
                    if (!tmp) { free(result); return NULL; }
                    result = tmp;
                }
                result[len++] = '$';
                result[len] = '\0';
                continue;
            }
            
            char *var_name = malloc(var_len + 1);
            if (!var_name) { free(result); return NULL; }
            memcpy(var_name, word + start, var_len);
            var_name[var_len] = '\0';
            
            char *val = getenv(var_name);
            free(var_name);
            
            if (val) {
                size_t val_len = strlen(val);
                while (len + val_len + 1 >= cap) {
                    cap *= 2;
                    char *tmp = realloc(result, cap);
                    if (!tmp) { free(result); return NULL; }
                    result = tmp;
                }
                strcpy(result + len, val);
                len += val_len;
            }
        } else {
            if (len + 2 >= cap) {
                cap *= 2;
                char *tmp = realloc(result, cap);
                if (!tmp) { free(result); return NULL; }
                result = tmp;
            }
            result[len++] = word[i++];
            result[len] = '\0';
        }
    }
    
    return result;
}
