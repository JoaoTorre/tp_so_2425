#include "manager.h"

int manager_fifo_fd;    // Variável global
pthread_mutex_t trinco; // Variável global

void sair()
{
    // Fechar e remover o FIFO
    close(manager_fifo_fd);
    unlink(MANAGER_FIFO);

    // Destruir o mutex
    pthread_mutex_destroy(&trinco);

    // Mensagem de encerramento
    printf("\n[MANAGER] - Motor desligado. Até breve!\n");

    // Use apenas return para encerrar o programa
    return;
}

void handle_signal(int sig, siginfo_t *info, void *ucontext)
{
    printf("\n[SINAL] - Recebido sinal %d. Encerrando...\n", sig);

    sair();
}

void Abrir_ManagerPipe(int *manager_fifo_fd)
{
    // Criar FIFO para o Manager
    if (mkfifo(MANAGER_FIFO, 0777) == -1)
    {
        perror("[ERRO] - FIFO já existe ou não foi possível criá-lo");
        exit(EXIT_FAILURE);
    }

    printf("\n[MANAGER] - FIFO criado com sucesso.\n");

    // Abrir FIFO em modo bloqueante
    if ((*manager_fifo_fd = open(MANAGER_FIFO, O_RDWR)) == -1)
    {
        perror("[ERRO] - Falha ao abrir FIFO para leitura/escrita");
        unlink(MANAGER_FIFO); // Limpar recursos
        exit(EXIT_FAILURE);
    }

    printf("\n[MANAGER] - FIFO aberto para leitura e escrita.\n");
}

int abre_ClientPipe(pid_t pidCliente)
{
    int clientpipe_fd;
    char pipe[100];

    sprintf(pipe, FEED_FIFO, pidCliente);

    // Abrir FIFO do cliente
    if ((clientpipe_fd = open(pipe, O_WRONLY)) == -1)
    {
        fprintf(stderr, "\n[ERRO] - Falha ao abrir FIFO do cliente (PID: %d).\n", pidCliente);
        return -1;
    }

    return clientpipe_fd;
}

void processa_pedido(int manager_fifo_fd, pthread_mutex_t *trinco)
{
    CLIENT *newClient;
    pedido_t pedido;
    resposta_t resposta;
    int feed_fifo_fd, nBytes_lidos, nBytes_escritos;

    // Ler pedido do Manager FIFO
    if ((nBytes_lidos = read(manager_fifo_fd, &pedido, sizeof(pedido))) == -1)
    {
        perror("[ERRO] - Falha ao ler do FIFO do Manager");
        return;
    }

    // Verificar comando do pedido
    if (strstr(pedido.comando, PEDIDO_PARA_JOGAR))
    {
        printf("\n[JOGADOR %d] Nome: %s\n", pedido.PidRemetente, pedido.nome_user);

        // Alocar novo cliente
        newClient = malloc(sizeof(CLIENT));
        if (!newClient)
        {
            fprintf(stderr, "[ERRO] - Falha ao alocar memória para o novo cliente.\n");
            return;
        }

        // Preencher dados do cliente
        pthread_mutex_lock(trinco);
        newClient->PidRemetente = pedido.PidRemetente;
        strcpy(newClient->user.nome, pedido.nome_user);
        newClient->user.pid = pedido.PidRemetente;
        newClient->user.registado = 1;
        newClient->nextClient = NULL;
        pthread_mutex_unlock(trinco);

        printf("[MANAGER] Novo cliente registrado: %s\n", newClient->user.nome);

        // Abrir FIFO do cliente
        if ((feed_fifo_fd = abre_ClientPipe(pedido.PidRemetente)) == -1)
        {
            free(newClient);
            return;
        }

        // Responder ao cliente
        strcpy(resposta.resposta, FEED_ACEITE);
        printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);

        if ((nBytes_escritos = write(feed_fifo_fd, &resposta, sizeof(resposta))) == -1)
        {
            perror("[ERRO] - Falha ao enviar resposta para o cliente");
        }

        close(feed_fifo_fd);
        free(newClient);
    }
}

int main(int argc, char *argv[])
{
    int continua = 1;
    fd_set fdset, fdset_backup;
    struct sigaction sa;

    // Inicializar mutex
    pthread_mutex_init(&trinco, NULL);

    // Configurar tratamento de sinais
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handle_signal;
    sigaction(SIGINT, &sa, NULL);

    // Criar e abrir FIFO do Manager
    Abrir_ManagerPipe(&manager_fifo_fd);

    // Inicializar conjuntos de descritores de arquivo
    FD_ZERO(&fdset);
    FD_SET(STDIN_FILENO, &fdset);
    FD_SET(manager_fifo_fd, &fdset);

    printf("\n[MANAGER] - Sistema iniciado. Aguardando comandos...\n");

    while (continua)
    {
        fdset_backup = fdset;

        int ret = select(manager_fifo_fd + 1, &fdset_backup, NULL, NULL, NULL);
        if (ret == -1)
        {
            perror("[ERRO] - Falha no select");
            sair();
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &fdset_backup)) // stdin
        {
            char entrada[100];
            if (fgets(entrada, sizeof(entrada), stdin))
            {
                entrada[strcspn(entrada, "\n")] = 0;
                if (strcmp(entrada, "exit") == 0)
                {
                    printf("\n[MANAGER] Comando recebido: desligar.\n");
                    sair();
                    continua = 0;
                }
                else
                {
                    printf("[COMANDO] - Comando não reconhecido: %s\n", entrada);
                }
            }
        }

        if (FD_ISSET(manager_fifo_fd, &fdset_backup)) // FIFO do Manager
        {
            processa_pedido(manager_fifo_fd, &trinco);
        }
    }

    sair(); // Garantir que o sistema seja desligado corretamente
    return 0;
}
