#include "manager.h"

int manager_fifo_fd;
pthread_mutex_t trinco;
pthread_mutex_t clientes_mutex; // Mutex para proteger a lista de clientes registrados
CLIENT *clientes_registrados = NULL;

<<<<<<< Updated upstream
=======

>>>>>>> Stashed changes
// Função para finalizar o sistema
void sair()
{
    close(manager_fifo_fd);
    unlink(MANAGER_FIFO);
    pthread_mutex_destroy(&trinco);
    printf("\n[MANAGER] - Motor desligado. Até breve!\n");
    exit(0);
}


// Função para verificar comandos
int VerificaComando(char* str){ 
    toUpperString(str);

     // Verifica se é o comando CLOSE    
    if (strcmp(str, "CLOSE") == 0) {
        printf("\n[MANAGER] Comando recebido: desligar.\n");
        sair();
        return 0;
     // Verifica se é o comando USERS    
    }else if(strcmp(str, "USERS") == 0){
        printf("A mostrar listagem de utilizadores...\n");

    // Verifica se é o comando TOPICS     
    }else if(strcmp(str, "TOPICS") == 0){
        printf("A mostrar a listagem de topicos...\n");

    // Verifica se é o comando REMOVE
    }else if (strncmp(str, "REMOVE ", 7) == 0 || strcmp(str, "REMOVE") == 0) {
        int i = 7;
        int encontrado = 0;
        int contador = 0;
        int len = strlen(str);
        char remove_str[50] = {0}; // Inicializa com zeros

        // Captura o utilizador
        while (i < len && str[i] != ' ' && contador < sizeof(remove_str) - 1) {
            remove_str[contador++] = str[i++];
        }
        remove_str[contador] = '\0';

        if (contador > 0)
            encontrado += 1;

        if (encontrado == 1) {
            printf("A remover utilizador %s ...\n", remove_str);
        } else {
            printf("Insira no formato: remove <username>\n");
        }

    // Verifica se é o comando SHOW    
    }else if ((strncmp(str, "SHOW ", 5) == 0) || strcmp(str, "SHOW") == 0){
        int i = 5;
        int encontrado = 0;
        int contador = 0;
        int len = strlen(str);
        char topico_str[50] = {0}; // Inicializa com zeros

        // Captura o topico
        while (i < len && str[i] != ' ' && contador < sizeof(topico_str) - 1) {
            topico_str[contador++] = str[i++];
        }
        topico_str[contador] = '\0';

        if (contador > 0)
            encontrado += 1;

        if (encontrado == 1) {
            printf("A mostrar topico %s ...\n", topico_str);
        } else {
            printf("Insira no formato: show <topico>\n");
        }

      // Verifica se é o comando LOCK       
      }else if ((strncmp(str, "LOCK ", 5) == 0) || strcmp(str, "LOCK") == 0){
            int i = 5;
            int encontrado = 0;
            int contador = 0;
            int len = strlen(str);
            char topico_str[50] = {0}; // Inicializa com zeros

            // Captura o topico
            while (i < len && str[i] != ' ' && contador < sizeof(topico_str) - 1) {
                topico_str[contador++] = str[i++];
            }
            topico_str[contador] = '\0';

            if (contador > 0)
                encontrado += 1;

            if (encontrado == 1) {
                printf("A bloquear topico %s ...\n", topico_str);
            } else {
                printf("Insira no formato: lock <topico>\n");
            }

      // Verifica se é o comando UNLOCK     
      }else if ((strncmp(str, "UNLOCK ", 7) == 0) || strcmp(str, "UNLOCK") == 0){
            int i = 7;
            int encontrado = 0;
            int contador = 0;
            int len = strlen(str);
            char topico_str[50] = {0}; // Inicializa com zeros

            // Captura o topico
            while (i < len && str[i] != ' ' && contador < sizeof(topico_str) - 1) {
                topico_str[contador++] = str[i++];
            }
            topico_str[contador] = '\0';

            if (contador > 0)
                encontrado += 1;

            if (encontrado == 1) {
                printf("A desbloquear topico %s ...\n", topico_str);
            } else {
                printf("Insira no formato: unlock <topico>\n");
            }
       } 

    // Comando não reconhecido
    else {
        printf("Comando não reconhecido: %s\n", str);
    }    
}

// Função para tratar sinais de encerramento
void handle_signal(int sig, siginfo_t *info, void *ucontext)
{
    printf("\n[SINAL] - Sinal Recebido %d. A encerrar...\n", sig);
    sair();
}

// Função para verificar se o cliente já foi registrado
int cliente_ja_registrado(pid_t pid)
{
    pthread_mutex_lock(&clientes_mutex);
    CLIENT *atual = clientes_registrados;
    while (atual)
    {
        if (atual->PidRemetente == pid)
        {
            pthread_mutex_unlock(&clientes_mutex);
            return 1; // Cliente já registrado
        }
        atual = atual->nextClient;
    }
    pthread_mutex_unlock(&clientes_mutex);
    return 0; // Cliente não registrado
}

// Função para adicionar cliente à lista de registrados
void adicionar_cliente_registrado(pid_t pid)
{
    CLIENT *novo_cliente = malloc(sizeof(CLIENT));
    if (!novo_cliente)
    {
        fprintf(stderr, "[ERRO] - Falha ao alocar memória para o cliente registrado.\n");
        return;
    }

    novo_cliente->PidRemetente = pid;
    novo_cliente->nextClient = NULL;

    pthread_mutex_lock(&clientes_mutex);
    novo_cliente->nextClient = clientes_registrados;
    clientes_registrados = novo_cliente;
    pthread_mutex_unlock(&clientes_mutex);
}

// Função para criar FIFO
void Abrir_ManagerPipe(int *manager_fifo_fd)
{
    if (mkfifo(MANAGER_FIFO, 0666) == -1)
    {
        perror("[ERRO] - FIFO já existe ou não foi possível criá-lo");
        exit(EXIT_FAILURE);
    }

    printf("\n[MANAGER] - FIFO criado com sucesso.\n");

    *manager_fifo_fd = open(MANAGER_FIFO, O_RDWR);
    if (*manager_fifo_fd == -1)
    {
        unlink(MANAGER_FIFO); // Limpar recursos
        exit(EXIT_FAILURE);
    }

    printf("\n[MANAGER] - FIFO aberto para leitura e escrita.\n");
}

// Função para abrir o pipe do cliente
int abre_ClientPipe(pid_t pidCliente)
{
    char pipe[100];
    sprintf(pipe, FEED_FIFO, pidCliente);

    printf("[INFO] - Abrindo FIFO do cliente (PATH: %s)...\n", pipe);

    int fifo_fd = open(pipe, O_RDWR);
    if (fifo_fd == -1)
    {
        perror("[ERRO] - Falha ao abrir FIFO");
    }
    return fifo_fd;
}

void *responder_feed(void *arg)
{
    int feed_fifo_fd;
    CLIENT *client = (CLIENT *)arg;
    resposta_t resposta;
    pedido_t pedido;
    int nBytes_lidos, nBytes_escritos;
    fd_set read_fds;
    struct timeval timeout;

    // Abrir FIFO do cliente
    feed_fifo_fd = abre_ClientPipe(client->PidRemetente);
    if (feed_fifo_fd == -1)
    {
        fprintf(stderr, "[ERRO] - Falha ao abrir FIFO do cliente (PID: %d).\n", client->PidRemetente);
        free(client);
        return NULL;
    }

    while (1)
    {
        // Inicializar o conjunto de descritores de leitura
        FD_ZERO(&read_fds);
        FD_SET(feed_fifo_fd, &read_fds);

        printf("[THREAD %d] - Aguardando dados no FIFO...\n", client->PidRemetente);

        int retval = select(feed_fifo_fd + 1, &read_fds, NULL, NULL, NULL);
        if (retval == -1)
        {
            perror("[ERRO] - Falha no select");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(feed_fifo_fd, &read_fds))
        {
            nBytes_lidos = read(feed_fifo_fd, &pedido, sizeof(pedido));
            if (nBytes_lidos == -1)
            {
                perror("[ERRO] - Falha ao ler do FIFO");
            }
            else
            {
                printf("[INFO] - Pedido lido: %s\n", pedido.comando);
                printf("[INFO] - Nbytes lidos: %d\n", nBytes_lidos);
                strcpy(resposta.resposta, FEED_ACEITE);
                printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);

                if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
                {
                    perror("[ERRO] - Falha ao enviar resposta para o cliente");
                }
            }
        }
        close(feed_fifo_fd);
        feed_fifo_fd = abre_ClientPipe(client->PidRemetente);
    }

    close(feed_fifo_fd);
    free(client);
    return NULL;
}

// Função para processar pedido do Manager
void processa_pedido(int manager_fifo_fd, pthread_mutex_t *trinco)
{
    CLIENT *newClient;
    pedido_t pedido;
    resposta_t resposta;
    int feed_fifo_fd, nBytes_lidos, nBytes_escritos;

    // Ler pedido do Manager FIFO
    nBytes_lidos = read(manager_fifo_fd, &pedido, sizeof(pedido));
    if (nBytes_lidos == -1)
    {
        perror("[ERRO] - Falha ao ler do FIFO do Manager");
        return;
    }

    if (strstr(pedido.comando, PEDIDO_PARA_JOGAR))
    {
        // Verificar se o cliente já foi registrado
        if (cliente_ja_registrado(pedido.PidRemetente))
        {
            printf("[MANAGER] Cliente já registrado: %d. Ignorando pedido.\n", pedido.PidRemetente);
            return; // Cliente já registrado, não processa mais no loop principal
        }

        // Alocar e registrar o cliente
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

        // Adicionar cliente à lista de registrados
        adicionar_cliente_registrado(pedido.PidRemetente);

        // Abrir FIFO do cliente
        feed_fifo_fd = abre_ClientPipe(pedido.PidRemetente);
        if (feed_fifo_fd == -1)
        {
            free(newClient);
            return;
        }

        // Enviar resposta para o cliente
        strcpy(resposta.resposta, FEED_ACEITE);
        printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);

        if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
        {
            perror("[ERRO] - Falha ao enviar resposta para o cliente");
        }

        // Criar a thread para responder ao feed do cliente
        pthread_t thread_feed;
        if (pthread_create(&thread_feed, NULL, responder_feed, (void *)newClient) != 0)
        {
            fprintf(stderr, "[ERRO] - Falha ao criar thread para responder ao feed do cliente\n");
            free(newClient);
        }
        else
        {
            pthread_detach(thread_feed); // Garantir que a thread será liberada automaticamente após terminar
        }
    }
    else
    {
        printf("\n[ERRO] - Comando inválido: %s\n", pedido.comando);
    }
}

// Função principal
int main(int argc, char *argv[])
{
    int continua = 1;
    fd_set fdset, fdset_backup;
    struct sigaction sa;

    pthread_mutex_init(&trinco, NULL);
    pthread_mutex_init(&clientes_mutex, NULL);

    // Configurar o tratamento de sinais
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handle_signal;
    sigaction(SIGINT, &sa, NULL);

    // Criar e abrir FIFO do Manager
    Abrir_ManagerPipe(&manager_fifo_fd);

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

         /* LINHA DE COMANDOS - ADMIN */   

        if (FD_ISSET(STDIN_FILENO, &fdset_backup))
        {
            char entrada[100];
            if (fgets(entrada, sizeof(entrada), stdin))
            {
                size_t len = strlen(entrada);
                if (len > 0 && entrada[len - 1] == '\n') {
                    entrada[len - 1] = '\0';
                }

                continua = VerificaComando(entrada);
            }
        }

        if (FD_ISSET(manager_fifo_fd, &fdset_backup))
        {
            processa_pedido(manager_fifo_fd, &trinco);
        }
    }

    sair();
    return 0;
}