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
#define IDENTIFIER_START   ">1 USD at  "
#define IDENTIFIER_END     " "

/* structs */
typedef struct {
    size_t len;
    char *buff;
} sizedbuff;

/* function declarations */
size_t got_data(char *buffer, size_t itemsize, size_t nitems, sizedbuff *userp);
int lbprate_print(CURL *curl);
int parse_x(char *str, const char *beforex, const char *afterx,
            size_t *x_offset, size_t *x_len);

size_t got_data(char *buffer, size_t itemsize, size_t nitems, sizedbuff *userp)
{
    size_t bytes = itemsize * nitems;
    userp->buff = realloc(userp->buff, userp->len + bytes + 1);
    memcpy(userp->buff + userp->len, buffer, bytes);
    userp->len += bytes;
    userp->buff[userp->len] = '\0';
    return bytes;
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

    if (parse_x(page.buff, IDENTIFIER_START, IDENTIFIER_END,
                &buy_offset, &buy_len)) {
        goto CLEANUP;
    }

    if (parse_x(page.buff + buy_offset + buy_len, IDENTIFIER_START, IDENTIFIER_END,
                &sell_offset, &sell_len)) {
        goto CLEANUP;
    }

    buy = malloc(buy_len + 1);
    sell = malloc(sell_len + 1);

    memcpy(buy, page.buff + buy_offset, buy_len);
    buy[buy_len] = '\0';
    memcpy(sell, page.buff + buy_offset + buy_len + sell_offset, sell_len);
    sell[sell_len] = '\0';

    printf("Lbprate: Buy %s / Sell %s\n", buy, sell);
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

int main(void)
{
    CURL *curl;
    int return_code;

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "[Error] Failed to init libcurl!\n");
        return 1;
    }

    return_code = lbprate_print(curl);
    curl_easy_cleanup(curl);
    return return_code;
}
