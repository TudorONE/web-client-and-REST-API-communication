#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>

// Functii pentru comenzi admin
void login_admin(int sockfd);
void logout_admin(int sockfd);
void get_users(int sockfd, char *target_username, int *exists);
void add_user(int sockfd);
void delete_user(int sockfd);

// Functii pentru comenzi user
void login(int sockfd);
void logout(int sockfd);
void get_access(int sockfd);

// Functii pentru filme
void get_movies(int sockfd);
void get_movie(int sockfd);
void add_movie(int sockfd);
void update_movie(int sockfd);
void delete_movie(int sockfd);

// Functii pentru colectii
void get_collections(int sockfd);
void get_collection(int sockfd);
void add_collection(int sockfd);
void delete_collection(int sockfd);
void add_movie_to_collection(int sockfd);
void delete_movie_from_collection(int sockfd);

#endif
