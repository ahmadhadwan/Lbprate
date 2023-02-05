/*
 * Author: Ahmad Hadwan.
 * Copyright (C) 2022 Ahmad Hadwan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: ahmadhadwan2004@gmail.com
 *
 */
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* macros */
#define LBPRATE_IDENTIFIER_START        "1 USD at  "
#define LBPRATE_IDENTIFIER_END          " "
#define LBPRATE_TIME_IDENTIFIER_START   "Updated "
#define LBPRATE_TIME_IDENTIFIER_END     "</"

/* structs */
typedef struct {
    size_t len;
    char *buff;
} sizedbuff;

/* function declarations */
static size_t got_data(char *buffer, size_t itemsize, size_t nitems,
                       void *userp);
static int lbprate_print(CURL *curl);
static void parse_args(int argc, char **argv);
static int parse_x(char *str, const char *beforex, const char *afterx,
                   size_t *x_offset, size_t *x_len);
static void usage(char *execname, int exit_code);

/* global variables */
static int verbose = 0;

size_t got_data(char *buffer, size_t itemsize, size_t nitems, void *userp)
{
    size_t bytes;
    sizedbuff *sb;

    bytes = itemsize * nitems;
    sb = userp;

    sb->buff = realloc(sb->buff, sb->len + bytes + 1);
    memcpy(sb->buff + sb->len, buffer, bytes);
    sb->len += bytes;
    sb->buff[sb->len] = '\0';
    return bytes;
}

int lbprate_print(CURL *curl)
{
    CURLcode result;
    sizedbuff page;
    size_t time_offset, time_len, buy_offset, buy_len, sell_offset, sell_len;
    char *time, *buy, *sell;

    page.len = 0;
    page.buff = NULL;

    curl_easy_setopt(curl, CURLOPT_URL, "https://lbprate.com/");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, got_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &page);

    result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        fprintf(stderr, "[Error] [lbprate.com] Download problem: %s\n",
                curl_easy_strerror(result));
        return 1;
    }

    time_offset = time_len = buy_offset = buy_len = sell_offset = sell_len = 0;

    if (parse_x(page.buff, LBPRATE_TIME_IDENTIFIER_START,
                LBPRATE_TIME_IDENTIFIER_END, &time_offset, &time_len)) {
        goto CLEANUP;
    }

    if (parse_x(page.buff + time_offset + time_len, LBPRATE_IDENTIFIER_START,
                LBPRATE_IDENTIFIER_END, &buy_offset, &buy_len)) {
        goto CLEANUP;
    }

    if (parse_x(page.buff + buy_offset + buy_len, LBPRATE_IDENTIFIER_START,
                LBPRATE_IDENTIFIER_END, &sell_offset, &sell_len)) {
        goto CLEANUP;
    }

    time = malloc(time_len + 1);
    buy = malloc(buy_len + 1);
    sell = malloc(sell_len + 1);

    memcpy(time, page.buff + time_offset, time_len);
    time[time_len] = '\0';
    memcpy(buy, page.buff + time_offset + time_len + buy_offset, buy_len);
    buy[buy_len] = '\0';
    memcpy(sell, page.buff + buy_offset + buy_len + sell_offset, sell_len);
    sell[sell_len] = '\0';

    printf(verbose ? "Lbprate: Updated %s: Buy %s / Sell %s\n" : "%s: %s/%s\n",
            time, buy, sell);

    free(page.buff);
    free(time);
    free(buy);
    free(sell);
    return 0;

CLEANUP:
    free(page.buff);
    fprintf(stderr, "[Error] Failed to parse lbprate.com's page!\n");
    return 1;
}

void parse_args(int argc, char **argv)
{
    char *execname;

    execname = argv[0];
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '-') {
            int arglen = strlen(argv[i]);
            if (arglen < 2) {
                printf("Expected flags after `-`\n");
                usage(execname, 1);
            }
            for (int j = 1; j < arglen; j++) {
                switch (argv[i][j])
                {
                    case 'v': verbose = 1; break;
                    default:
                        printf("Invalid option: `-%c`\n", argv[i][j]);
                        usage(execname, 1);
                }
            }
        }
        else if (!strcmp(argv[i], "--help")) {
            usage(execname, 0);
        }
        else if (!strcmp(argv[i], "--verbose")) {
            verbose = 1;
        }
        else {
            printf("Invalid option: `%s`\n", argv[i]);
            usage(execname, 1);
        }
    }
}

int parse_x(char *str, const char *beforex, const char *afterx,
            size_t *x_offset, size_t *x_len)
{
    int beforex_len, afterx_len;

    beforex_len = strlen(beforex);
    afterx_len = strlen(afterx);

    for (size_t i = 0; str[i]; i++) {
        if (str[i] == beforex[0]) {
            if (!strncmp(str + i, beforex, beforex_len)) {
                i += beforex_len;
                *x_offset = i;
                do {
                    while (str[i] != afterx[0] && str[i] != '\0') {
                        i++;
                    }
                } while (strncmp(str + i, afterx, afterx_len));

                *x_len = i - *x_offset;
                return 0;
            }
        }
    }

    return 1;
}

void usage(char *execname, int exit_code)
{
    printf("Usage: %s [options]\n"
         "Options:\n"
         "  --help         Display this information.\n"
         "  --verbose, -v  Prints a verbose message.\n",
         execname
    );
    exit(exit_code);
}

int main(int argc, char **argv)
{
    CURL *curl;
    int return_code;

    parse_args(argc, argv);

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "[Error] Failed to init libcurl!\n");
        return 1;
    }

    return_code = lbprate_print(curl);
    curl_easy_cleanup(curl);
    return return_code;
}
