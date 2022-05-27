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
#define LBPRATE_IDENTIFIER_START   "1 USD at  "
#define LBPRATE_IDENTIFIER_END     " "

#define GTOG_IDENTIFIER_START      "GTOG&#039;s rate for its cards is "
#define GTOG_IDENTIFIER_END        " "

/* structs */
typedef struct {
    size_t len;
    char *buff;
} sizedbuff;

/* function declarations */
static size_t got_data(char *buffer, size_t itemsize, size_t nitems,
                       sizedbuff *userp);
static int gtog_print(CURL *curl);
static int lbprate_print(CURL *curl);
static int parse_x(char *str, const char *beforex, const char *afterx,
                   size_t *x_offset, size_t *x_len);
static void usage();

/* global variables */
static int display_gtog = 0;
static int verbose = 0;

size_t got_data(char *buffer, size_t itemsize, size_t nitems, sizedbuff *userp)
{
    size_t bytes = itemsize * nitems;
    userp->buff = realloc(userp->buff, userp->len + bytes + 1);
    memcpy(userp->buff + userp->len, buffer, bytes);
    userp->len += bytes;
    userp->buff[userp->len] = '\0';
    return bytes;
}

int gtog_print(CURL *curl)
{
    CURLcode result;
    sizedbuff page;
    size_t buy_offset, buy_len;
    char *buy;

    page.len = 0;
    page.buff = NULL;

    curl_easy_setopt(curl, CURLOPT_URL, "https://www.omt.com.lb/en/services/payment/o-store");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, got_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &page);

    result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        fprintf(stderr, "[Error] [omt.com] Download problem: %s\n", curl_easy_strerror(result));
        return 1;
    }

    buy_offset = buy_len = 0;

    if (parse_x(page.buff, GTOG_IDENTIFIER_START, GTOG_IDENTIFIER_END,
                &buy_offset, &buy_len)) {
        free(page.buff);
        fprintf(stderr, "[Error] Failed to parse omt.com's page!\n");
        return 1;
    }

    buy = malloc(buy_len + 1);

    memcpy(buy, page.buff + buy_offset, buy_len);
    buy[buy_len] = '\0';

    printf("GTOG: Buy %s\n", buy);
    free(page.buff);
    free(buy);
    return 0;
}

int lbprate_print(CURL *curl)
{
    CURLcode result;
    sizedbuff page;
    size_t buy_offset, buy_len, sell_offset, sell_len;
    char *buy, *sell;

    page.len = 0;
    page.buff = NULL;

    curl_easy_setopt(curl, CURLOPT_URL, "https://lbprate.com/");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, got_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &page);

    result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        fprintf(stderr, "[Error] [lbprate.com] Download problem: %s\n", curl_easy_strerror(result));
        return 1;
    }

    buy_offset = buy_len = sell_offset = sell_len = 0;

    if (parse_x(page.buff, LBPRATE_IDENTIFIER_START, LBPRATE_IDENTIFIER_END,
                &buy_offset, &buy_len)) {
        goto CLEANUP;
    }

    if (parse_x(page.buff + buy_offset + buy_len, LBPRATE_IDENTIFIER_START, LBPRATE_IDENTIFIER_END,
                &sell_offset, &sell_len)) {
        goto CLEANUP;
    }

    buy = malloc(buy_len + 1);
    sell = malloc(sell_len + 1);

    memcpy(buy, page.buff + buy_offset, buy_len);
    buy[buy_len] = '\0';
    memcpy(sell, page.buff + buy_offset + buy_len + sell_offset, sell_len);
    sell[sell_len] = '\0';

    if (verbose) {
        printf("Lbprate: Buy %s / Sell %s\n", buy, sell);
    }
    else {
        printf("%s/%s\n", buy, sell);
    }

    free(page.buff);
    free(buy);
    free(sell);
    return 0;

CLEANUP:
    free(page.buff);
    fprintf(stderr, "[Error] Failed to parse lbprate.com's page!\n");
    return 1;
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

void usage()
{
    puts("Usage: lbprate [options]\n"
         "Options:\n"
         "  --gtog      Fetch GTOG buy rate.\n"
         "  --help      Display this information.\n"
         "  --verbose   Display the rate's sources."
    );
}

int main(int argc, char **argv)
{
    CURL *curl;
    int return_code;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--gtog")) {
            display_gtog = 1;
        }
        else if (!strcmp(argv[i], "--help")) {
            usage();
            return 0;
        }
        else if (!strcmp(argv[i], "--verbose")) {
            verbose = 1;
        }
        else {
            usage();
            return 1;
        }
    }

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "[Error] Failed to init libcurl!\n");
        return 1;
    }

    return_code = lbprate_print(curl);
    if (display_gtog) {
        return_code |= gtog_print(curl);
    }

    curl_easy_cleanup(curl);
    return return_code;
}
