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
    if (mkfifo(feedPipe, 0777) != 0)
    {
        fprintf(stderr, "\n[ERRO] - Erro ao criar FIFO feed\n");
        exit(-1);
    }

    if ((*FeedPipe_fd = open(feedPipe, O_RDWR)) == -1)
    {
        fprintf(stderr, "\n[ERRO] - Erro ao criar FIFO Feed\n");
        exit(-1);
    }
}

void handle_signal(int sig, siginfo_t *info, void *ucontext)
{
    // Apenas printar e sair
    printf("\n[SINAL RECEBIDO]: Encerrando o jogo...\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    int res;
    int continua;
    char entrada[TAM_MSG];
    char nome[TAM_NOME];
    int nBytes_escritos, nBytes_lidos;
    int Managerpipe_fd = -1;
    int FeedPipe_fd = -1;
    CLIENT user;
    struct sigaction sa;
    pedido_t pedidoRegisto;
    resposta_t resposta;

    user.nextClient = NULL;

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
    strncpy(nome, argv[1], TAM_NOME - 1);
    nome[TAM_NOME - 1] = '\0';

    strcpy(pedidoRegisto.nome_user, nome);
    strcpy(pedidoRegisto.comando, PEDIDO_PARA_JOGAR);
    printf("Sessão iniciada\nNome de utilizador: %s\n", pedidoRegisto.nome_user);
    continua = 1;

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
        strcpy(user.user.nome, pedidoRegisto.nome_user);
        printf("\n[RESPOSTA]: %s\n", resposta.resposta);
    }

    sair(Managerpipe_fd, FeedPipe_fd);
    return 0;
}
