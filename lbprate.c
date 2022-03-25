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
    char* str;
} string;

size_t
got_data(char *buffer, size_t itemsize, size_t nitems, string *userp)
{
    size_t bytes = itemsize * nitems;
    userp->str = realloc(userp->str, userp->len + bytes + 1);
    memcpy(userp->str + userp->len, buffer, bytes);
    userp->len += bytes;
    userp->str[userp->len] = '\0';
    return bytes;
}

int
parse_rates(string *page, char *buy, char *sell)
{
    size_t start;

    buy[0] = 0;
    sell[0] = 0;

    for (size_t i = 0; i < page->len; i++) {
        if (page->str[i] == IDENTIFIER_START) {
            i++;
            if (!strncmp(page->str + i, IDENTIFIER, IDENTIFIER_SIZE)) {
                i += IDENTIFIER_SIZE;
                start = i;
                while (page->str[i] != IDENTIFIER_END && page->str[i] != '\0') i++;
                if (buy[0] == 0) {
                    memcpy(buy, page->str + start, i - start);
                    buy[i - start] = 0;
                }
                else if (sell[0] == 0) {
                    memcpy(sell, page->str + start, i - start);
                    sell[i - start] = 0;
                    return 0;
                }
                else {
                    fprintf(stderr, "[ERROR] Identifier is not unique!\n");
                    return 1;
                }
            }
        }
    }

    return 1;
}

int
main(void)
{
    CURL *curl;
    CURLcode result;
    string page;
    char buy[256], sell[256];

    page.len = 0;
    page.str = malloc(0);

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
        free(page.str);
        return 1;
    }

    curl_easy_cleanup(curl);

    if (parse_rates(&page, buy, sell)) {
        free(page.str);
        fprintf(stderr, "[ERROR] Failed to parse the page!\n");
        return 1;
    }

    printf("%s/%s\n", buy, sell);
    free(page.str);

    return 0;
}
