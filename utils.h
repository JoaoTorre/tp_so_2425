#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

// maximo de utilizadores
#define MAX_USERS 10

// maximo de topicos
#define MAX_TOPICS 20

// maximo mensagens persistentes por topico
#define MAX_TOPICS_MSG_PERSIS 5

// maximo de caracteres no nome do utilizador
#define TAM_NOME 20

// maximo de caracteres de corpo de mensagem
#define TAM_MSG

// nome do FIFO do manager
#define MANAGER_FIFO "/tmp/manager_fifo"

// nome do FIFO para cada feed
#define FEED_FIFO "/tmp/feed_%d_fifo"

// ---- MENSAGENS DEFAULT ----

#define FEED_ACEITE "Utilizador registado com sucesso\n"
#define FEED_RECUSADO "Nome de utilizador já em uso\n"
#define FEED_EXPULSO "Foi expulso pelo manager\n"
#define FEED_ESPERA "Maximo de utilizadores atingido. Por favor aguarde\n"

#define TOPICO_BLOQUEADO "O topico encontra-se bloqueado\n"
#define TOPICO_MAXIMO "Maximo de topicos atingido\n"
#define TOPICO_MAX_MSG_PERSISTENTES "Maximo de mensagens persistentes referentes a este topico atingido\n"

// estrutura de jogador
typedef struct
{
    pid_t pid;
    char username[TAM_NOME];
    pthread_t th;
    pthread_mutex_t *user_trinco;
} utilizador;

void toUpperString(char *str);

#endif