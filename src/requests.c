#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "requests.h"
#include "helpers.h"

char *compute_get_request(char *host, char *url, char *query_params,
                          char **cookies, int cookies_count, char *auth_token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }
    compute_message(message, line);

    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (auth_token != NULL) {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer %s", auth_token);
        compute_message(message, line);
    }

    if (cookies != NULL && cookies_count > 0) {
        memset(line, 0, LINELEN);
        strcat(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i != cookies_count - 1) {
                strcat(line, "; ");
            }
        }
        compute_message(message, line);
    }

    compute_message(message, "");

    free(line);
    return message;
}

char *compute_post_request(char *host, char *url, char *content_type,
                           char *body_data, char **cookies, int cookies_count,
                           char *auth_token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    sprintf(line, "Content-Length: %ld", strlen(body_data));
    compute_message(message, line);

    if (auth_token != NULL) {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer %s", auth_token);
        compute_message(message, line);
    }

    if (cookies != NULL && cookies_count > 0) {
        memset(line, 0, LINELEN);
        strcat(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i != cookies_count - 1) {
                strcat(line, "; ");
            }
        }
        compute_message(message, line);
    }

    compute_message(message, "");

    compute_message(message, body_data);

    free(line);
    return message;
}

char *compute_delete_request(char *host, char *url, char *query_params,
                             char **cookies, int cookies_count, char **headers) {
    char *message = calloc(BUFLEN, sizeof(char));
    char line[LINELEN];

    if (query_params != NULL) {
        sprintf(line, "DELETE %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "DELETE %s HTTP/1.1", url);
    }
    compute_message(message, line);

    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (headers != NULL) {
        for (int i = 0; headers[i] != NULL; i++) {
            compute_message(message, headers[i]);
        }
    }

    if (cookies != NULL && cookies_count > 0) {
        strcat(message, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(message, cookies[i]);
            if (i < cookies_count - 1) {
                strcat(message, "; ");
            }
        }
        strcat(message, "\r\n");
    }

    strcat(message, "\r\n");
    return message;
}

char *compute_put_request(char *host, char *url, char *content_type,
                          char *body_data, char **cookies, int cookies_count,
                          char *auth_token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    sprintf(line, "PUT %s HTTP/1.1", url);
    compute_message(message, line);

    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    sprintf(line, "Content-Length: %ld", strlen(body_data));
    compute_message(message, line);

    if (auth_token != NULL) {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer %s", auth_token);
        compute_message(message, line);
    }

    if (cookies != NULL && cookies_count > 0) {
        memset(line, 0, LINELEN);
        strcat(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i != cookies_count - 1) {
                strcat(line, "; ");
            }
        }
        compute_message(message, line);
    }

    compute_message(message, "");
    compute_message(message, body_data);

    free(line);
    return message;
}
