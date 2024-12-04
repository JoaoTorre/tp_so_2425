#include "feed.h"

void sair(int Managerpipe_fd, int FeedPipe_fd)
{
    char nomePipe[100];
    sprintf(nomePipe, FEED_FIFO, getpid());
    unlink(nomePipe);

    // Fechar os pipes abertos
    if (Managerpipe_fd != -1)
        close(Managerpipe_fd);
    if (FeedPipe_fd != -1)
        close(FeedPipe_fd);

    printf("\nFeed desligado\n\n");
    exit(0);
}

void criarPipe_Manager(int *Managerpipe_fd)
{
    if (access(MANAGER_FIFO, F_OK) == -1)
    {
        fprintf(stderr, "\n[ERRO] - Não foi encontrado nenhum Manager ligado \n");
        exit(-1);
    }

    if ((*Managerpipe_fd = open(MANAGER_FIFO, O_WRONLY)) == -1)
    {
        fprintf(stderr, "\n[ERRO] - Tentativa falhada ao abrir servidor\n");
        exit(1);
    }
}

void criarPipe_Feed(int *FeedPipe_fd)
{
    char feedPipe[50];
    sprintf(feedPipe, FEED_FIFO, getpid());

    // Garantir que o FIFO é recriado
    unlink(feedPipe);
    if (mkfifo(feedPipe, 0666) != 0)
    {
        perror("[ERRO] - Erro ao criar FIFO feed");
        exit(-1);
    }

    // Abrir o FIFO com permissões de leitura e escrita
    *FeedPipe_fd = open(feedPipe, O_RDWR);
    if (*FeedPipe_fd == -1)
    {
        perror("[ERRO] - Erro ao abrir FIFO Feed");
        exit(-1);
    }

    printf("[INFO] - Abrindo PIPE do cliente (PATH: %s)...\n", feedPipe);
}

void handle_signal(int sig, siginfo_t *info, void *ucontext)
{
    // Apenas printar e sair
    printf("\n[SINAL RECEBIDO]: A encerrar o jogo...\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    int maxFd, n;
    char entrada[TAM_MSG], tempComando[TAM_MSG];
    int nBytes_escritos, nBytes_lidos;
    int Managerpipe_fd = -1;
    int FeedPipe_fd = -1;
    struct sigaction sa;
    pedido_t pedidoRegisto, pedido;
    resposta_t resposta;
    fd_set fdset;
    mensagem_t mensagem;

    if (argc != 2)
    {
        fprintf(stderr, "[ERRO] - Nº de argumentos invalido: './feed <username>'\n");
        exit(-1);
    }

    criarPipe_Manager(&Managerpipe_fd);
    criarPipe_Feed(&FeedPipe_fd);

    /* Configurar sinais */
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handle_signal;
    sigaction(SIGINT, &sa, NULL);

    // Guardar o nome do utilizador
    strncpy(pedidoRegisto.nome_user, argv[1], TAM_NOME - 1);
    pedidoRegisto.nome_user[TAM_NOME - 1] = '\0'; // Garantir terminação nula
    strncpy(pedidoRegisto.comando, PEDIDO_PARA_JOGAR, TAM_NOME - 1);
    pedidoRegisto.comando[TAM_NOME - 1] = '\0';

    printf("Sessão iniciada\nNome de utilizador: %s\n", pedidoRegisto.nome_user);

    /* Enviar dados ao servidor */
    pedidoRegisto.PidRemetente = getpid();
    nBytes_escritos = write(Managerpipe_fd, &pedidoRegisto, sizeof(pedidoRegisto));
    if (nBytes_escritos == -1)
    {
        perror("[ERRO] ao escrever no pipe Manager");
        sair(Managerpipe_fd, FeedPipe_fd);
    }

    // Ler resposta
    nBytes_lidos = read(FeedPipe_fd, &resposta, sizeof(resposta));
    if (nBytes_lidos == -1)
    {
        perror("[ERRO] ao ler do pipe Feed");
        sair(Managerpipe_fd, FeedPipe_fd);
    }

    if (strstr(resposta.resposta, FEED_ACEITE) != NULL)
    {
        printf("\n[RESPOSTA]: %s\n", resposta.resposta);
    }

    while (1)
    {
        FD_ZERO(&fdset); // Reinicializar o fdset
        FD_SET(STDIN_FILENO, &fdset);
        FD_SET(FeedPipe_fd, &fdset);

        maxFd = (STDIN_FILENO > FeedPipe_fd) ? STDIN_FILENO : FeedPipe_fd;
        printf("[FEED] - Introduz um comando: ");
        fflush(stdout);

        if ((n = select(maxFd + 1, &fdset, NULL, NULL, NULL)) == -1)
        {
            perror("[ERRO] - Erro no select");
            sair(Managerpipe_fd, FeedPipe_fd);
        }

        if (FD_ISSET(STDIN_FILENO, &fdset))
        { // DADOS DO STDIN
            if ((n = read(STDIN_FILENO, entrada, sizeof(entrada) - sizeof(char))) == -1)
            {
                fprintf(stderr, "\n[ERRO] - Erro na leitura do stdin\n");
                sair(Managerpipe_fd, FeedPipe_fd);
            }

            entrada[n - 1] = '\0';
            fflush(stdin);

            strcpy(tempComando, entrada);

            char *comando = strtok(tempComando, " ");
            char *argumento1 = strtok(NULL, " ");
            char *argumento2 = strtok(NULL, " ");
            char *argumento3 = NULL;

            if (comando != NULL)
            {
                toUpperString(comando);
            }
            if (argumento1 != NULL)
            {
                toUpperString(argumento1);
            }

            if (argumento1 != NULL && argumento2 != NULL)
            {
                argumento3 = entrada + (argumento2 - entrada) + strlen(argumento2) + 1;

                if (*argumento3 == '\0')
                {
                    argumento3 = NULL;
                }
            }

            memset(&pedido, 0, sizeof(pedido));
            memset(&mensagem, 0, sizeof(mensagem_t));

            if (strcmp(comando, "EXIT") == 0)
            {
                printf("COMANDO SAIR\n");
                strcpy(pedido.comando, comando);
                nBytes_escritos = write(FeedPipe_fd, &pedido, sizeof(pedido));
                if (nBytes_escritos == -1)
                {
                    perror("[ERRO] ao escrever no pipe Feed");
                    sair(Managerpipe_fd, FeedPipe_fd);
                }
                printf("[INFO] - nBytes_escritos: %d\n", nBytes_escritos);
                printf("[INFO] - Tamanho da estrutura pedido: %zu bytes\n", sizeof(pedido));
                break; // Garantir que o loop seja encerrado após o envio do comando
            }
            else if (strcmp(comando, "MSG") == 0)
            {
                if (argumento1 == NULL || argumento2 == NULL || argumento3 == NULL)
                {
                    printf("\n[ERRO] - Comando inválido. Formato esperado: 'MSG <tópico> <duracao> <mensagem>'\n");
                }
                else
                {

                    strcpy(pedido.mensagem.topico, argumento1);
                    pedido.mensagem.duracao = atoi(argumento2);
                    strcpy(pedido.mensagem.mensagem, argumento3);
                    pedido.PidRemetente = getpid();
                    strcpy(pedido.comando, comando);

                    // Enviar a mensagem para o Manager pelo FIFO do cliente
                    printf("Dados do pedido:\n");
                    printf("Duracao: %d\n", pedido.mensagem.duracao);
                    printf("Mensagem: %s\n", pedido.mensagem.mensagem);
                    printf("PidRemetente: %d\n", pedido.PidRemetente);
                    printf("Comando: %s\n", pedido.comando);

                    nBytes_escritos = write(FeedPipe_fd, &pedido, sizeof(pedido));
                    if (nBytes_escritos == -1)
                    {
                        perror("[ERRO] ao escrever no pipe Feed");
                        sair(Managerpipe_fd, FeedPipe_fd);
                    }
                    printf("[INFO] - nBytes_escritos: %d\n", nBytes_escritos);
                    printf("[INFO] - Tamanho da estrutura pedido: %zu bytes\n", sizeof(pedido));
                }
            }
            else if (strcmp(comando, "SUBSCRIBE") == 0)
            {
                if (argumento1 != NULL && argumento2 == NULL && argumento3 == NULL)
                {
                    printf("\n[COMANDO SUBSCRIBE] - Subscribindo ao tópico '%s'\n", argumento1);
                    // Enviar a subscrição para o Manager, se necessário
                    strcpy(pedido.mensagem.topico, argumento1);

                    strcpy(pedido.comando, comando);
                    nBytes_escritos = write(FeedPipe_fd, &pedido, sizeof(pedido));
                    if (nBytes_escritos == -1)
                    {
                        perror("[ERRO] ao escrever no pipe Feed");
                        sair(Managerpipe_fd, FeedPipe_fd);
                    }
                    printf("[INFO] - nBytes_escritos: %d\n", nBytes_escritos);
                    printf("[INFO] - Tamanho da estrutura pedido: %zu bytes\n", sizeof(pedido));
                }
                else
                {
                    printf("\n[ERRO] - Falta o nome do tópico após 'SUBSCRIBE'\n");
                }
            }
            else if (strcmp(comando, "TOPICS") == 0)
            {
                printf("COMANDO TOPICS\n");
                strcpy(pedido.comando, comando);
                nBytes_escritos = write(FeedPipe_fd, &pedido, sizeof(pedido));
                if (nBytes_escritos == -1)
                {
                    perror("[ERRO] ao escrever no pipe Feed");
                    sair(Managerpipe_fd, FeedPipe_fd);
                }
                printf("[INFO] - nBytes_escritos: %d\n", nBytes_escritos);
                printf("[INFO] - Tamanho da estrutura pedido: %zu bytes\n", sizeof(pedido));
            }
            else if (strcmp(comando, "UNSUBSCRIBE") == 0)
            {
                if (argumento1 != NULL && argumento2 == NULL && argumento3 == NULL)
                {
                    printf("\n[COMANDO unsubscribe] - Subscribindo ao tópico '%s'\n", argumento1);
                    // Enviar a subscrição para o Manager, se necessário
                    strcpy(pedido.mensagem.topico, argumento1);

                    strcpy(pedido.comando, comando);
                    nBytes_escritos = write(FeedPipe_fd, &pedido, sizeof(pedido));
                    if (nBytes_escritos == -1)
                    {
                        perror("[ERRO] ao escrever no pipe Feed");
                        sair(Managerpipe_fd, FeedPipe_fd);
                    }
                    printf("[INFO] - nBytes_escritos: %d\n", nBytes_escritos);
                    printf("[INFO] - Tamanho da estrutura pedido: %zu bytes\n", sizeof(pedido));
                }

                else
                {
                    printf("\n[ERRO] - Falta o nome do tópico após 'unsubscribe'\n");
                }
            }
            else
            {
                printf("\n[ERRO] - Comando '%s' não reconhecido\n", entrada);
            }
        }

        if (FD_ISSET(FeedPipe_fd, &fdset))
        {
            nBytes_lidos = read(FeedPipe_fd, &resposta, sizeof(resposta));
            if (nBytes_lidos > 0)
            {
                printf("[FEED] - Resposta recebida: %s\n", resposta.resposta);
            }
            else if (nBytes_lidos == -1)
            {
                perror("[ERRO] - Falha ao ler do FIFO Feed");
                sair(Managerpipe_fd, FeedPipe_fd);
            }
        }
    }
    sair(Managerpipe_fd, FeedPipe_fd);
    return 0;
}