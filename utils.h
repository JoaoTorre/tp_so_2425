#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> // Necessário para pthread_t e pthread_mutex_t

// Constantes para o sistema
#define MAX_USERS 10            // Máximo de utilizadores
#define MAX_TOPICS 20           // Máximo de tópicos
#define MAX_TOPICS_MSG_PERSIS 5 // Máximo de mensagens persistentes por tópico
#define TAM_NOME 100            // Máximo de caracteres no nome do utilizador
#define TAM_MSG 300             // Máximo de caracteres de corpo de mensagem
#define TOPIC_LENGTH 100        // Máximo de caracteres no nome do tópico

// Nome do FIFO do manager
#define MANAGER_FIFO "/tmp/manager_fifo"

// Nome do FIFO para cada feed
#define FEED_FIFO "/tmp/feed_%d_fifo"

// ---- MENSAGENS DEFAULT ----
#define FEED_ACEITE "Utilizador registado com sucesso\n"
#define FEED_RECUSADO "Nome de utilizador já em uso\n"
#define FEED_EXPULSO "Foi expulso pelo manager\n"
#define FEED_ESPERA "Maximo de utilizadores atingido. Por favor aguarde\n"
#define PEDIDO_PARA_JOGAR "Jogador pede para jogar\n"

#define TOPICO_BLOQUEADO "O topico encontra-se bloqueado\n"
#define TOPICO_MAXIMO "Maximo de topicos atingido\n"
#define TOPICO_MAX_MSG_PERSISTENTES "Maximo de mensagens persistentes referentes a este topico atingido\n"
#define TOPICO_SUBSCRITO "Topico subscrito com sucesso\n"
#define TOPICO_UNSUBSCRIBE "Topico removido com sucesso\n"
#define TOPICO_NAO_SUBSCRITO "O topico nao se encontra subscrito\n"
#define TOPICO_NAO_EXISTE "O topico nao existe\n"
#define TOPICO_JA_SUBSCRITO "O topico ja se encontra subscrito\n"

// ---- COMANDOS ----
#define TOPICS "TOPICS"
#define SUBSCRIBE "SUBSCRIBE"
#define UNSUBSCRIBE "UNSUBSCRIBE"
#define MSG "MSG"
#define EXIT "EXIT"

// ---- ESTRUTURAS DE DADOS ----

typedef struct
{
    char nome[TOPIC_LENGTH];
    int num_mensagens;
    enum
    {
        DESBLOQUEADO,
        BLOQUEADO
    } estado;
    // int estado; // 1 -> desbloqueado, 0 -> bloqueado
} topico;

// Estrutura para mensagens
typedef struct
{
    char topico[TAM_NOME];  // Tópico da mensagem
    int duracao;            // Duração da mensagem
    char mensagem[TAM_MSG]; // Corpo da mensagem
} mensagem_t;

// estrutura para guardar topicos
typedef struct
{
    topico topicos[MAX_TOPICS];
    int num_topicos;
} topicos_t;

// Estrutura para pedidos (mensagem enviada por um utilizador)
typedef struct
{
    pid_t PidRemetente;       // PID do remetente
    char nome_user[TAM_NOME]; // Nome do jogador
    char comando[TAM_NOME];   // Comando enviado
    mensagem_t mensagem;      // Mensagem enviada
} pedido_t;

// Estrutura para representar um utilizador
typedef struct
{
    topico topicos[MAX_TOPICS]; // Tópicos subscritos
    char nome[TAM_NOME];        // Nome do utilizador
    int num_topicos;            // Número de tópicos subscritos
} utilizador;

// Estrutura de lista ligada de clientes
typedef struct client
{
    pid_t PidRemetente;        // PID do remetente
    utilizador user;           // Dados do utilizador
    struct client *nextClient; // Ponteiro para o próximo cliente na lista
} CLIENT, *pCLIENT;

typedef struct
{
    char resposta[TAM_MSG];
} resposta_t;

// estrutura para guardar utilizadores no sistema
typedef struct
{
    CLIENT *clientes_registrados;
    int num_utilizadores;
} users_t;

// estrutura para guardar mensagens persistentes
typedef struct
{
    mensagem_t mensagens[MAX_TOPICS_MSG_PERSIS];
    int num_mensagens;
} mensagens_persist_t;

// ---- DECLARAÇÕES DE FUNÇÕES ----
void toUpperString(char *str);

#endif
