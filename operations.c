#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "operations.h"

#define MAX_LINE_LENGTH 4096

// Вспомогательная функция для замены всех вхождений в строке
// Возвращает новую строку (которую нужно потом free) или NULL, если замен не было
char* replace_all(const char *source, regex_t *regex, const char *replacement) {
    char *result = NULL;
    char *cursor = (char *)source;
    regmatch_t match;
   // size_t result_len = 0;
    size_t result_cap = 0;

    // Сначала проверим, есть ли совпадения, и сформируем новую строку
    while (regexec(regex, cursor, 1, &match, 0) == 0) {
        // match.rm_so - начало совпадения, match.rm_eo - конец
        size_t prefix_len = match.rm_so;
        size_t replace_len = strlen(replacement);
        size_t suffix_start = match.rm_eo;

        // Рассчитываем необходимый размер
        size_t new_part_len = prefix_len + replace_len;
        
        // Увеличиваем буфер результата
        if (result == NULL) {
            result_cap = new_part_len + strlen(cursor + suffix_start) + 1; // +1 для \0
            result = malloc(result_cap);
            result[0] = '\0';
        } else {
            result_cap += new_part_len; // Грубая оценка, можно оптимизировать
            result = realloc(result, result_cap + strlen(cursor + suffix_start) + 100);
        }

        // Копируем часть до совпадения
        strncat(result, cursor, prefix_len);
        // Копируем замену
        strcat(result, replacement);

        // Сдвигаем курсор дальше после совпадения
        cursor += suffix_start;
       // result_len = strlen(result);
    }

    // Если были замены, доливаем остаток строки
    if (result != NULL) {
        strcat(result, cursor);
        return result;
    }

    return NULL; // Замен не было
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

    // --- ПАРСИНГ ---
    char type = ' '; 
    char pattern_str[256] = {0};
    char replacement[256] = {0};

    // Простой парсер аргументов (s/regex/repl/ или /regex/d)
    if (command[0] == '/') {
        // /pattern/d
        size_t len = strlen(command);
        if (command[len - 1] == 'd') {
            type = 'd';
            strncpy(pattern_str, command + 1, len - 3);
        }
    } else if (command[0] == 's' && command[1] == '/') {
        // s/pattern/replacement/
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

    // --- КОМПИЛЯЦИЯ REGEX ---
    regex_t regex;
    int ret = regcomp(&regex, pattern_str, REG_EXTENDED);
    if (ret) {
        fprintf(stderr, "Could not compile regex\n");
        fclose(src); fclose(dest); remove(temp_filename);
        return;
    }

    // --- ПОСТРОЧНАЯ ОБРАБОТКА ---
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), src)) {
        // Удаляем символ новой строки для обработки (вернем его при записи, если нужно)
        // Но проще работать с полной строкой.
        
        if (type == 'd') {
            // Если находим совпадение - НЕ пишем строку (удаляем)
            if (regexec(&regex, line, 0, NULL, 0) == 0) {
                continue; // Пропускаем строку
            }
            fputs(line, dest);
        } 
        else if (type == 's') {
            // Замена
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
        // На Windows rename может не сработать, если файл существует. Сначала удалим.
        remove(filename);
        if (rename(temp_filename, filename) != 0) {
            perror("Error renaming file");
        }
    }
}