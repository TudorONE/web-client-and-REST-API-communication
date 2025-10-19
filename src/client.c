#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

#define HOST "63.32.125.183"
#define PORT 8081

char *admin_cookie = NULL;
char *user_cookie = NULL;

char *token = NULL;

void login_admin(int sockfd) {
    if (admin_cookie != NULL) {
        printf("ERROR: Admin deja autentificat\n");
        return;
    }

    char username[100], password[100];
    printf("username=");
    fflush(stdout);
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';

    printf("password=");
    fflush(stdout);
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';

    char body[BUFLEN];
    sprintf(body, "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);

    char *request = compute_post_request(HOST, "/api/v1/tema/admin/login",
                        "application/json", body, NULL, 0, NULL);
    send_to_server(sockfd, request);

    char *response = receive_from_server(sockfd);

    if (strstr(response, "200 OK") && strstr(response, "Set-Cookie: ")) {
        char *cookie_start = strstr(response, "Set-Cookie: ");
        cookie_start += strlen("Set-Cookie: ");
        char *cookie_end = strstr(cookie_start, ";");
        if (cookie_end) {
            size_t len = cookie_end - cookie_start;
            admin_cookie = calloc(len + 1, sizeof(char));
            strncpy(admin_cookie, cookie_start, len);
        }

        printf("SUCCESS: Admin autentificat cu succes\n");

    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Credențiale invalide\n");
    } else if (strstr(response, "409 CONFLICT")) {
        printf("ERROR: Admin deja autentificat\n");
    }

    free(request);
    free(response);
}

void logout_admin(int sockfd) {
    if (admin_cookie == NULL) {
        printf("ERROR: Nu există sesiune activă de admin\n");
        return;
    }

    char *cookies[] = {admin_cookie};
    char *request = compute_get_request(HOST, "/api/v1/tema/admin/logout",NULL, cookies, 1, NULL);
    send_to_server(sockfd, request);

    char *response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Admin delogat\n");

        free(admin_cookie);
        admin_cookie = NULL;
    }

    free(request);
    free(response);
}


void get_users(int sockfd, char *target_username, int *exists) {
    if (admin_cookie == NULL) {
        printf("ERROR: Nu există sesiune activă de admin\n");
        return;
    }

    char *cookies[] = {admin_cookie};
    char *request = compute_get_request(HOST, "/api/v1/tema/admin/users", NULL, cookies, 1, NULL);
    send_to_server(sockfd, request);

    char *response = receive_from_server(sockfd);

    if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Lipsă permisiuni admin\n");

        free(request);
        free(response);
        return;
    }

    char *json = basic_extract_json_response(response);
    if (json != NULL) {
        JSON_Value *val = json_parse_string(json);
        JSON_Object *obj = json_value_get_object(val);
        JSON_Array *users = json_object_get_array(obj, "users");

        printf("SUCCESS: Lista utilizatorilor\n");

        for (size_t i = 0; i < json_array_get_count(users); ++i) {
            JSON_Object *user = json_array_get_object(users, i);
            const char *username = json_object_get_string(user, "username");
            const char *password = json_object_get_string(user, "password");
            int id = (int)json_object_get_number(user, "id");
            printf("#%d %s:%s\n", id, username, password);

            if (target_username != NULL && strcmp(username, target_username) == 0) {
                *exists = 1;
            }
        }

        json_value_free(val);
    }

    free(request);
    free(response);
}

void add_user(int sockfd) {
    if (admin_cookie == NULL) {
        printf("ERROR: Nu există sesiune activă de admin\n");
        return;
    }

    char username[100], password[100];
    printf("username=");
    fflush(stdout);
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';

    printf("password=");
    fflush(stdout);
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';

    char body[BUFLEN];
    sprintf(body, "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);

    char *cookies[] = {admin_cookie};
    char *request = compute_post_request(HOST, "/api/v1/tema/admin/users",
                                         "application/json", body, cookies, 1, NULL);
    send_to_server(sockfd, request);

    char *response = receive_from_server(sockfd);

    if (strstr(response, "201 CREATED")) {
        printf("SUCCESS: Utilizator adăugat cu succes\n");
    } else if (strstr(response, "409 CONFLICT")) {
        printf("ERROR: Utilizatorul există deja\n");
    } else if (strstr(response, "400 BAD REQUEST")) {
        printf("ERROR: Date invalide\n");
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Lipsă permisiuni admin\n");
    }

    free(request);
    free(response);
}

void delete_user(int sockfd) {
    if (admin_cookie == NULL) {
        printf("ERROR: Nu există sesiune activă de admin\n");
        return;
    }

    char username[100];
    printf("username=");
    fflush(stdout);
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';

    char url[LINELEN];
    sprintf(url, "/api/v1/tema/admin/users/%s", username);

    char *cookies[] = {admin_cookie};
    char *request = compute_delete_request(HOST, url, NULL, cookies, 1, NULL);
    send_to_server(sockfd, request);

    char *response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Utilizator șters\n");
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Lipsă permisiuni admin\n");
    } else if (strstr(response, "400 BAD REQUEST")) {
        printf("ERROR: Username invalid\n");
    }

    free(request);
    free(response);
}

void login(int sockfd) {
    if (user_cookie != NULL) {
        printf("ERROR: Utilizator deja autentificat\n");
        return;
    }

    char admin_username[100], username[100], password[100];
    
    printf("admin_username=");
    fflush(stdout);
    fgets(admin_username, sizeof(admin_username), stdin);
    admin_username[strcspn(admin_username, "\n")] = '\0';

    printf("username=");
    fflush(stdout);
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';

    printf("password=");
    fflush(stdout);
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';

    char body[BUFLEN];
    sprintf(body,
        "{\"admin_username\":\"%s\",\"username\":\"%s\",\"password\":\"%s\"}",
        admin_username, username, password);

    char *request = compute_post_request(HOST, "/api/v1/tema/user/login",
                        "application/json", body, NULL, 0, NULL);
    send_to_server(sockfd, request);

    char *response = receive_from_server(sockfd);

    if (strstr(response, "200 OK") && strstr(response, "Set-Cookie: ")) {
        char *cookie_start = strstr(response, "Set-Cookie: ");
        cookie_start += strlen("Set-Cookie: ");
        char *cookie_end = strstr(cookie_start, ";");
        if (cookie_end) {
            size_t len = cookie_end - cookie_start;
            user_cookie = calloc(len + 1, sizeof(char));
            strncpy(user_cookie, cookie_start, len);
        }

        printf("SUCCESS: Autentificare reușită\n");

    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Credențiale invalide (admin sau utilizator)\n");
    } else if (strstr(response, "409 CONFLICT")) {
        printf("ERROR: Utilizator deja autentificat\n");
    }

    free(request);
    free(response);
}

void logout(int sockfd) {
    if (user_cookie == NULL) {
        printf("ERROR: Nu există sesiune activă de utilizator\n");
        return;
    }

    char *cookies[] = {user_cookie};
    char *request = compute_get_request(HOST, "/api/v1/tema/user/logout", NULL, cookies, 1, NULL);
    send_to_server(sockfd, request);

    char *response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Utilizator delogat\n");

        free(user_cookie);
        user_cookie = NULL;
    }

    free(request);
    free(response);
}

void get_access(int sockfd) {
    if (user_cookie == NULL) {
        printf("ERROR: Nu există sesiune activă de utilizator\n");
        return;
    }

    char *cookies[] = {user_cookie};
    char *request = compute_get_request(HOST, "/api/v1/tema/library/access", NULL, cookies, 1, NULL);
    send_to_server(sockfd, request);

    char *response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        char *json = basic_extract_json_response(response);
        if (json != NULL) {
            JSON_Value *val = json_parse_string(json);
            JSON_Object *obj = json_value_get_object(val);
            const char *jwt = json_object_get_string(obj, "token");

            if (jwt != NULL) {
                if (token != NULL) {
                    free(token);
                }
                token = calloc(strlen(jwt) + 1, sizeof(char));
                strcpy(token, jwt);

                printf("SUCCESS: Token JWT primit\n");
            }

            json_value_free(val);
        }
    }

    free(request);
    free(response);
}

void get_movies(int sockfd) {
    if (token == NULL) {
        printf("ERROR: Fără acces library\n");
        return;
    }

    char *request = compute_get_request(HOST, "/api/v1/tema/library/movies", NULL, NULL, 0, token);
    send_to_server(sockfd, request);

    char *response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        char *json = basic_extract_json_response(response);
        if (json != NULL) {
            JSON_Value *val = json_parse_string(json);
            JSON_Object *obj = json_value_get_object(val);
            JSON_Array *movies = json_object_get_array(obj, "movies");

            printf("SUCCESS: Lista filmelor\n");

            for (size_t i = 0; i < json_array_get_count(movies); ++i) {
                JSON_Object *movie = json_array_get_object(movies, i);
                int id = (int)json_object_get_number(movie, "id");
                const char *title = json_object_get_string(movie, "title");

                printf("#%d %s\n", id, title);
            }

            json_value_free(val);
        }
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Lipsă permisiuni (JWT invalid)\n");
    }

    free(request);
    free(response);
}

void get_movie(int sockfd) {
    if (user_cookie == NULL || token == NULL) {
        printf("ERROR: Fără acces library\n");
        return;
    }

    char id[20];
    printf("id=");
    fflush(stdout);
    fgets(id, sizeof(id), stdin);
    id[strcspn(id, "\n")] = '\0';

    char url[LINELEN];
    sprintf(url, "/api/v1/tema/library/movies/%s", id);

    char *request = compute_get_request(HOST, url, NULL, NULL, 0, token);
    send_to_server(sockfd, request);

    char *response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        char *json = basic_extract_json_response(response);
        if (json != NULL) {
            JSON_Value *val = json_parse_string(json);
            JSON_Object *obj = json_value_get_object(val);

            const char *title = json_object_get_string(obj, "title");
            int year = (int)json_object_get_number(obj, "year");
            const char *description = json_object_get_string(obj, "description");
            const char *rating_str = json_object_get_string(obj, "rating");
            double rating = atof(rating_str);

            printf("SUCCESS: Detalii film\n");
            printf("title: %s\n", title);
            printf("year: %d\n", year);
            printf("description: %s\n", description);
            printf("rating: %.1f\n", rating);

            json_value_free(val);
        }
    } else if (strstr(response, "404 NOT FOUND")) {
        printf("ERROR: ID invalid\n");
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Lipsă permisiuni (JWT invalid)\n");
    }

    free(request);
    free(response);
}

void add_movie(int sockfd) {
    if (token == NULL) {
        printf("ERROR: Fără acces library\n");
        return;
    }

    char title[100], year[10], description[256], rating_str[10];
    double rating;

    printf("title=");
    fflush(stdout);
    fgets(title, sizeof(title), stdin);
    title[strcspn(title, "\n")] = '\0';

    printf("year=");
    fflush(stdout);
    fgets(year, sizeof(year), stdin);
    year[strcspn(year, "\n")] = '\0';

    printf("description=");
    fflush(stdout);
    fgets(description, sizeof(description), stdin);
    description[strcspn(description, "\n")] = '\0';

    printf("rating=");
    fflush(stdout);
    fgets(rating_str, sizeof(rating_str), stdin);
    rating = atof(rating_str);

    char body[BUFLEN];
    snprintf(body, BUFLEN,
        "{\"title\":\"%s\",\"year\":%s,\"description\":\"%s\",\"rating\":%.1f}",
        title, year, description, rating);

    char *request = compute_post_request(HOST, "/api/v1/tema/library/movies",
                                         "application/json", body, NULL, 0, token);
    send_to_server(sockfd, request);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "201 CREATED")) {
        printf("SUCCESS: Film adăugat\n");
    } else if (strstr(response, "400 BAD REQUEST")) {
        printf("ERROR: Date invalide\n");
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Lipsă permisiuni (JWT invalid)\n");
    }

    free(request);
    free(response);
}

void update_movie(int sockfd) {
    if (token == NULL) {
        printf("ERROR: Fără acces library\n");
        return;
    }

    char id[20], title[100], year[10], description[256], rating_str[10];
    double rating;

    printf("id=");
    fflush(stdout);
    fgets(id, sizeof(id), stdin);
    id[strcspn(id, "\n")] = '\0';

    printf("title=");
    fflush(stdout);
    fgets(title, sizeof(title), stdin);
    title[strcspn(title, "\n")] = '\0';

    printf("year=");
    fflush(stdout);
    fgets(year, sizeof(year), stdin);
    year[strcspn(year, "\n")] = '\0';

    printf("description=");
    fflush(stdout);
    fgets(description, sizeof(description), stdin);
    description[strcspn(description, "\n")] = '\0';

    printf("rating=");
    fflush(stdout);
    fgets(rating_str, sizeof(rating_str), stdin);
    rating = atof(rating_str);

    char url[LINELEN];
    sprintf(url, "/api/v1/tema/library/movies/%s", id);

    char body[BUFLEN];
    snprintf(body, BUFLEN,
        "{\"title\":\"%s\",\"year\":%s,\"description\":\"%s\",\"rating\":%.1f}",
        title, year, description, rating);

    char *request = compute_put_request(HOST, url, "application/json", body, NULL, 0, token);
    send_to_server(sockfd, request);

    char *response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Film actualizat\n");
    } else if (strstr(response, "400 BAD REQUEST")) {
        printf("ERROR: Date invalide\n");
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Lipsă permisiuni (JWT invalid)\n");
    } else if (strstr(response, "404 NOT FOUND")) {
        printf("ERROR: ID invalid\n");
    }

    free(request);
    free(response);
}

void delete_movie(int sockfd) {
    if (token == NULL) {
        printf("ERROR: Fără acces library\n");
        return;
    }

    char id[20];
    printf("id=");
    fflush(stdout);
    fgets(id, sizeof(id), stdin);
    id[strcspn(id, "\n")] = '\0';

    char url[LINELEN];
    snprintf(url, LINELEN, "/api/v1/tema/library/movies/%s", id);

    char auth_header[LINELEN];
    snprintf(auth_header, LINELEN, "Authorization: Bearer %s", token);
    char *headers[] = {auth_header, NULL};

    char *request = compute_delete_request(HOST, url, NULL, NULL, 0, headers);
    send_to_server(sockfd, request);

    char *response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Film șters cu succes\n");
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Lipsă permisiuni (JWT invalid)\n");
    } else if (strstr(response, "404 NOT FOUND")) {
        printf("ERROR: ID invalid\n");
    }

    free(request);
    free(response);
}

void get_collections(int sockfd) {
    if (token == NULL) {
        printf("ERROR: Fără acces library\n");
        return;
    }

    char *request = compute_get_request(HOST, "/api/v1/tema/library/collections", NULL, NULL, 0, token);
    send_to_server(sockfd, request);

    char *response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        char *json = basic_extract_json_response(response);
        if (json != NULL) {
            JSON_Value *val = json_parse_string(json);
            JSON_Object *obj = json_value_get_object(val);
            JSON_Array *collections = json_object_get_array(obj, "collections");

            printf("SUCCESS: Lista colecțiilor\n");

            for (size_t i = 0; i < json_array_get_count(collections); ++i) {
                JSON_Object *collection = json_array_get_object(collections, i);
                int id = (int)json_object_get_number(collection, "id");
                const char *title = json_object_get_string(collection, "title");

                printf("#%d: %s\n", id, title);
            }

            json_value_free(val);
        }
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Lipsă permisiuni (JWT invalid)\n");
    }

    free(request);
    free(response);
}

void get_collection(int sockfd) {
    if (token == NULL) {
        printf("ERROR: Fără acces library\n");
        return;
    }

    char id[20];
    printf("id=");
    fflush(stdout);
    fgets(id, sizeof(id), stdin);
    id[strcspn(id, "\n")] = '\0';

    char url[LINELEN];
    snprintf(url, LINELEN, "/api/v1/tema/library/collections/%s", id);

    char *request = compute_get_request(HOST, url, NULL, NULL, 0, token);
    send_to_server(sockfd, request);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        char *json = basic_extract_json_response(response);
        if (json != NULL) {
            JSON_Value *val = json_parse_string(json);
            JSON_Object *obj = json_value_get_object(val);

            const char *title = json_object_get_string(obj, "title");
            const char *owner = json_object_get_string(obj, "owner");
            JSON_Array *movies = json_object_get_array(obj, "movies");

            printf("SUCCESS: Detalii colecție\n");
            printf("title: %s\n", title);
            printf("owner: %s\n", owner);

            for (size_t i = 0; i < json_array_get_count(movies); i++) {
                JSON_Object *movie = json_array_get_object(movies, i);
                int movie_id = (int)json_object_get_number(movie, "id");
                const char *movie_title = json_object_get_string(movie, "title");
                printf("#%d: %s\n", movie_id, movie_title);
            }

            json_value_free(val);
        }
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Lipsă permisiuni (JWT invalid)\n");
    } else if (strstr(response, "404 NOT FOUND")) {
        printf("ERROR: ID invalid\n");
    }

    free(request);
    free(response);
}

void add_collection(int sockfd) {
    if (token == NULL) {
        printf("ERROR: Fără acces library\n");
        return;
    }

    char title[100];
    char num_str[10];
    int num_movies;

    printf("title=");
    fflush(stdout);
    fgets(title, sizeof(title), stdin);
    title[strcspn(title, "\n")] = '\0';

    printf("num_movies=");
    fflush(stdout);
    fgets(num_str, sizeof(num_str), stdin);
    num_movies = atoi(num_str);

    int movie_ids[num_movies];
    for (int i = 0; i < num_movies; i++) {
        char prompt[30], value[20];
        sprintf(prompt, "movie_id[%d]=", i);
        printf("%s", prompt);
        fflush(stdout);
        fgets(value, sizeof(value), stdin);
        movie_ids[i] = atoi(value);
    }

    char body[BUFLEN];
    snprintf(body, BUFLEN, "{\"title\":\"%s\"}", title);

    char *request = compute_post_request(HOST, "/api/v1/tema/library/collections",
                                         "application/json", body, NULL, 0, token);
    send_to_server(sockfd, request);
    char *response = receive_from_server(sockfd);

    int collection_id = -1;
    if (strstr(response, "201 CREATED")) {
        char *json = basic_extract_json_response(response);
        if (json) {
            JSON_Value *val = json_parse_string(json);
            JSON_Object *obj = json_value_get_object(val);
            collection_id = (int)json_object_get_number(obj, "id");
            json_value_free(val);
        }
    } else {
        if (strstr(response, "400 BAD REQUEST")) {
            printf("ERROR: Date invalide\n");
        } else if (strstr(response, "403 FORBIDDEN")) {
            printf("ERROR: Lipsă permisiuni (JWT invalid)\n");
        }
        free(request);
        free(response);
        return;
    }

    free(request);
    free(response);

    int failed = 0;

    for (int i = 0; i < num_movies; i++) {
        char url[LINELEN];
        snprintf(url, LINELEN, "/api/v1/tema/library/collections/%d/movies", collection_id);

        char movie_body[BUFLEN];
        snprintf(movie_body, BUFLEN, "{\"id\":%d}", movie_ids[i]);

        char *movie_request = compute_post_request(HOST, url, "application/json",
                                                   movie_body, NULL, 0, token);
        send_to_server(sockfd, movie_request);
        char *movie_response = receive_from_server(sockfd);

        if (strstr(movie_response, "200 OK")) {
            continue;
        } else if (strstr(movie_response, "404 NOT FOUND")) {
            printf("ERROR: Film #%d - date invalide\n", movie_ids[i]);
            failed = 1;
        } else if (strstr(movie_response, "403 FORBIDDEN")) {
            printf("ERROR: Film #%d - lipsă permisiuni\n", movie_ids[i]);
            failed = 1;
        }

        free(movie_request);
        free(movie_response);

        if (failed) {
            char url_delete[LINELEN];
            snprintf(url_delete, LINELEN, "/api/v1/tema/library/collections/%d", collection_id);
            char auth_header[LINELEN];
            snprintf(auth_header, LINELEN, "Authorization: Bearer %s", token);
            char *headers[] = {auth_header, NULL};

            char *delete_request = compute_delete_request(HOST, url_delete, NULL, NULL, 0, headers);
            send_to_server(sockfd, delete_request);
            char *delete_response = receive_from_server(sockfd);
            free(delete_request);
            free(delete_response);

            return;
        }
    }

    printf("SUCCESS: Colecție adăugată\n");
}

void delete_collection(int sockfd) {
    if (token == NULL) {
        printf("ERROR: Fără acces library\n");
        return;
    }

    char id_str[20];
    printf("id=");
    fflush(stdout);
    fgets(id_str, sizeof(id_str), stdin);
    id_str[strcspn(id_str, "\n")] = '\0';

    char url[LINELEN];
    snprintf(url, LINELEN, "/api/v1/tema/library/collections/%s", id_str);

    char auth_header[LINELEN];
    snprintf(auth_header, LINELEN, "Authorization: Bearer %s", token);
    char *headers[] = {auth_header, NULL};

    char *request = compute_delete_request(HOST, url, NULL, NULL, 0, headers);
    send_to_server(sockfd, request);

    char *response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Colecție ștearsă\n");
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Lipsă permisiuni (JWT invalid) sau nu sunteți owner\n");
    } else if (strstr(response, "404 NOT FOUND")) {
        printf("ERROR: ID invalid\n");
    }

    free(request);
    free(response);
}

void add_movie_to_collection(int sockfd) {
    if (token == NULL) {
        printf("ERROR: Fără acces library\n");
        return;
    }

    char collection_id_str[20], movie_id_str[20];
    int collection_id, movie_id;

    printf("collection_id=");
    fflush(stdout);
    fgets(collection_id_str, sizeof(collection_id_str), stdin);
    collection_id = atoi(collection_id_str);

    printf("movie_id=");
    fflush(stdout);
    fgets(movie_id_str, sizeof(movie_id_str), stdin);
    movie_id = atoi(movie_id_str);

    char url[LINELEN];
    snprintf(url, LINELEN, "/api/v1/tema/library/collections/%d/movies", collection_id);

    char body[BUFLEN];
    snprintf(body, BUFLEN, "{\"id\":%d}", movie_id);

    char *request = compute_post_request(HOST, url, "application/json", body, NULL, 0, token);
    send_to_server(sockfd, request);

    char *response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Film adăugat în colecție\n");
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Lipsă permisiuni\n");
    } else if (strstr(response, "400 BAD REQUEST")) {
        printf("ERROR: Date invalide\n");
    }

    free(request);
    free(response);
}

void delete_movie_from_collection(int sockfd) {
    if (token == NULL) {
        printf("ERROR: Fără acces library\n");
        return;
    }

    char collection_id_str[20], movie_id_str[20];
    int collection_id, movie_id;

    printf("collection_id=");
    fflush(stdout);
    fgets(collection_id_str, sizeof(collection_id_str), stdin);
    collection_id = atoi(collection_id_str);

    printf("movie_id=");
    fflush(stdout);
    fgets(movie_id_str, sizeof(movie_id_str), stdin);
    movie_id = atoi(movie_id_str);

    char url[LINELEN];
    snprintf(url, LINELEN, "/api/v1/tema/library/collections/%d/movies/%d", collection_id, movie_id);

    char auth_header[LINELEN];
    snprintf(auth_header, LINELEN, "Authorization: Bearer %s", token);
    char *headers[] = {auth_header, NULL};

    char *request = compute_delete_request(HOST, url, NULL, NULL, 0, headers);
    send_to_server(sockfd, request);

    char *response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Film șters din colecție\n");
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Lipsă permisiuni (JWT invalid) sau nu sunteți owner\n");
    } else if (strstr(response, "404 NOT FOUND")) {
        printf("ERROR: ID invalid sau filmul nu este în colecție\n");
    }

    free(request);
    free(response);
}

int main() {
    char command[100];

    while (1) {
        int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';

        if (strcmp(command, "login_admin") == 0) {
            login_admin(sockfd);
        } else if (strcmp(command, "logout_admin") == 0) {
            logout_admin(sockfd);
        } else if (strcmp(command, "get_users") == 0) {
            int dummy = 0;
            get_users(sockfd, NULL, &dummy);
        } else if (strcmp(command, "add_user") == 0) {
            add_user(sockfd);
        } else if (strcmp(command, "delete_user") == 0) {
            delete_user(sockfd);
        } else if (strcmp(command, "login") == 0) {
            login(sockfd);
        } else if (strcmp(command, "logout") == 0) {
            logout(sockfd);
        } else if (strcmp(command, "get_access") == 0) {
            get_access(sockfd);
        } else if (strcmp(command, "get_movies") == 0) {
            get_movies(sockfd);
        } else if (strcmp(command, "get_movie") == 0) {
            get_movie(sockfd);
        } else if (strcmp(command, "add_movie") == 0) {
            add_movie(sockfd);
        } else if (strcmp(command, "update_movie") == 0) {
            update_movie(sockfd);
        } else if (strcmp(command, "delete_movie") == 0) {
            delete_movie(sockfd);
        } else if (strcmp(command, "get_collections") == 0) {
            get_collections(sockfd);
        } else if (strcmp(command, "get_collection") == 0) {
            get_collection(sockfd);
        } else if (strcmp(command, "add_collection") == 0) {
            add_collection(sockfd);
        } else if (strcmp(command, "delete_collection") == 0) {
            delete_collection(sockfd);
        } else if (strcmp(command, "add_movie_to_collection") == 0) {
            add_movie_to_collection(sockfd);
        } else if (strcmp(command, "delete_movie_from_collection") == 0) {
            delete_movie_from_collection(sockfd);
        } else if (strcmp(command, "exit") == 0) {
            close_connection(sockfd);
            break;
        } else {
            printf("ERROR: Comandă necunoscută\n");
        }

        close_connection(sockfd);
    }

    free(admin_cookie);
    free(user_cookie);
    free(token);
    return 0;
}
