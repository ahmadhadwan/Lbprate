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
#define IDENTIFIER_START   '>'
#define IDENTIFIER_END     ' '
#define IDENTIFIER         "1 USD at  "
#define IDENTIFIER_SIZE    10

/* structs */
typedef struct {
    size_t len;
    char *buff;
} sizedbuff;

size_t got_data(char *buffer, size_t itemsize, size_t nitems, sizedbuff *userp)
{
    size_t bytes = itemsize * nitems;
    userp->buff = realloc(userp->buff, userp->len + bytes + 1);
    memcpy(userp->buff + userp->len, buffer, bytes);
    userp->len += bytes;
    userp->buff[userp->len] = '\0';
    return bytes;
}

int parse_rates(sizedbuff *page, char *buy, char *sell)
{
    size_t start;

    buy[0] = 0;
    sell[0] = 0;

    for (size_t i = 0; i < page->len; i++) {
        if (page->buff[i] == IDENTIFIER_START) {
            i++;
            if (!strncmp(page->buff + i, IDENTIFIER, IDENTIFIER_SIZE)) {
                i += IDENTIFIER_SIZE;
                start = i;
                while (page->buff[i] != IDENTIFIER_END
                    && page->buff[i] != '\0') {
                    i++;
                }
                
                if (buy[0] == 0) {
                    memcpy(buy, page->buff + start, i - start);
                    buy[i - start] = 0;
                }
                else {
                    memcpy(sell, page->buff + start, i - start);
                    sell[i - start] = 0;
                    return 0;
                }
            }
        }
    }

    return 1;
}

int main(void)
{
    CURL *curl;
    CURLcode result;
    sizedbuff page;
    char buy[256], sell[256];

    page.len = 0;
    page.buff = NULL;

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "[ERROR] Failed to init libcurl!\n");
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, "https://lbprate.com/");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, got_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &page);

    result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        fprintf(stderr, "[ERROR] Download problem: %s\n", curl_easy_strerror(result));
        curl_easy_cleanup(curl);
        free(page.buff);
        return 1;
    }

    curl_easy_cleanup(curl);

    if (parse_rates(&page, buy, sell)) {
        free(page.buff);
        fprintf(stderr, "[ERROR] Failed to parse the page!\n");
        return 1;
    }

    printf("Lbprate: %s/%s\n", buy, sell);
    free(page.buff);

    return 0;
}
