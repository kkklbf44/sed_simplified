#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "operations.h"

#define MAX_LINE_LENGTH 4096
char* replace_all(const char *source, regex_t *regex, const char *replacement) {
    char *result = NULL;
    char *cursor = (char *)source;
    regmatch_t match;
    size_t result_cap = 0;
    while (regexec(regex, cursor, 1, &match, 0) == 0) {
        size_t prefix_len = match.rm_so;
        size_t replace_len = strlen(replacement);
        size_t suffix_start = match.rm_eo;

        size_t new_part_len = prefix_len + replace_len;
        
        if (result == NULL) {
            result_cap = new_part_len + strlen(cursor + suffix_start) + 1; // +1 для \0
            result = malloc(result_cap);
            result[0] = '\0';
        } else {
            result_cap += new_part_len; 
            result = realloc(result, result_cap + strlen(cursor + suffix_start) + 100);
        }

        strncat(result, cursor, prefix_len);
        strcat(result, replacement);

        cursor += suffix_start;
    }

    if (result != NULL) {
        strcat(result, cursor);
        return result;
    }

    return NULL; 
}

void process_sed_command(const char *filename, const char *command) {
    FILE *src = fopen(filename, "r");
    if (!src) {
        perror("Error opening source file");
        return;
    }

    char temp_filename[256];
    snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", filename);
    FILE *dest = fopen(temp_filename, "w");
    if (!dest) {
        perror("Error opening temp file");
        fclose(src);
        return;
    }

    char type = ' '; 
    char pattern_str[256] = {0};
    char replacement[256] = {0};

    if (command[0] == '/') {
        size_t len = strlen(command);
        if (command[len - 1] == 'd') {
            type = 'd';
            strncpy(pattern_str, command + 1, len - 3);
        }
    } else if (command[0] == 's' && command[1] == '/') {
        type = 's';
        const char *second = strchr(command + 2, '/');
        if (second) {
            strncpy(pattern_str, command + 2, second - (command + 2));
            const char *third = strchr(second + 1, '/');
            if (third) {
                strncpy(replacement, second + 1, third - (second + 1));
            } else {
                strcpy(replacement, second + 1);
            }
        }
    }

    if (type == ' ') {
        fprintf(stderr, "Invalid command format. Use s/old/new/ or /old/d\n");
        fclose(src); fclose(dest); remove(temp_filename);
        return;
    }

    regex_t regex;
    int ret = regcomp(&regex, pattern_str, REG_EXTENDED);
    if (ret) {
        fprintf(stderr, "Could not compile regex\n");
        fclose(src); fclose(dest); remove(temp_filename);
        return;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), src)) {

        
        if (type == 'd') {
            if (regexec(&regex, line, 0, NULL, 0) == 0) {
                continue; 
            }
            fputs(line, dest);
        } 
        else if (type == 's') {
            char *new_line = replace_all(line, &regex, replacement);
            if (new_line) {
                fputs(new_line, dest);
                free(new_line);
            } else {
                fputs(line, dest);
            }
        }
    }

    regfree(&regex);
    fclose(src);
    fclose(dest);

    if (rename(temp_filename, filename) != 0) {
        remove(filename);
        if (rename(temp_filename, filename) != 0) {
            perror("Error renaming file");
        }
    }

}
