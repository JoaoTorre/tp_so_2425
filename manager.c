#include "manager.h"

int manager_fifo_fd;
int CONTROLADOR = 1;
pthread_mutex_t mensagens_mutex; // Mutex para proteger a lista de mensagens
pthread_mutex_t topicos_mutex;   // Mutex para proteger a lista de topicos
pthread_mutex_t clientes_mutex;  // Mutex para proteger a lista de clientes registados
users_t users;
users_t listaEspera; // Lista de espera para os utilizadores
topicos_t topicos;
mensagens_persist_t mensagens;

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

void avisarTodos()
{
    pthread_mutex_lock(&clientes_mutex);
    CLIENT *atual = users.clientes_registrados;
    while (atual)
    {
        int feed_fifo_fd = abre_ClientPipe(atual->PidRemetente);
        if (feed_fifo_fd != -1)
        {
            resposta_t resposta;
            strcpy(resposta.resposta, EXIT);

            if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
            {
                perror("[ERRO] - Falha ao enviar mensagem EXIT para o feed");
            }
            close(feed_fifo_fd);
        }
        atual = atual->nextClient;
    }
    pthread_mutex_unlock(&clientes_mutex);

    printf("[MANAGER] - Mensagem 'EXIT' enviada a todos os utilizadores registrados.\n");
}

// Função para sair do sistema
void sair()
{
    avisarTodos();
    close(manager_fifo_fd);
    unlink(MANAGER_FIFO);
    pthread_mutex_destroy(&clientes_mutex);
    pthread_mutex_destroy(&mensagens_mutex);
    pthread_mutex_destroy(&topicos_mutex);
    printf("\n[MANAGER] - Motor desligado. Até breve!\n");
    CONTROLADOR = 0;
}

// Função para tratar sinais
void handle_signal(int sig, siginfo_t *info, void *ucontext)
{
    printf("\n[SINAL] - Sinal Recebido %d. A encerrar...\n", sig);
    sair();
}

// Função para verificar se o tópico já existe
int verifica_topico(char topico[])
{
    pthread_mutex_lock(&topicos_mutex);
    for (int i = 0; i < topicos.num_topicos; i++)
    {
        if (strcmp(topico, topicos.topicos[i].nome) == 0) // Corrigido o acesso
        {
            pthread_mutex_unlock(&topicos_mutex);
            return 1; // Tópico existe
        }
    }

    pthread_mutex_unlock(&topicos_mutex);
    return 0; // Tópico não existe
}

// Função para Mostrar Mensagens de Tópico
void Mostrar_MensagensTopico(char topico[])
{

    if (verifica_topico(topico))
    {
        for (int i = 0; i < mensagens.num_mensagens; i++)
        {
            if (strcmp(mensagens.mensagens[i].topico, topico) == 0)
            {
                printf("Mensagem: %s (Duração: %d)\n", mensagens.mensagens[i].mensagem, mensagens.mensagens[i].duracao);
            }
        }
    }
    else
    {
        printf("[MANAGER] topico não registado\n");
    }
}

// Função para verificar se o nome já está a ser usado
int verifica_nome(char nome_user[])
{
    pthread_mutex_lock(&clientes_mutex);
    CLIENT *atual = users.clientes_registrados;
    while (atual)
    {
        if (strcmp(atual->user.nome, nome_user) == 0)
        {
            pthread_mutex_unlock(&clientes_mutex);
            return 1; // nome já registrado
        }
        atual = atual->nextClient;
    }
    pthread_mutex_unlock(&clientes_mutex);
    return 0; // nome não registrado
}

// Função para adicionar cliente à lista de registrados
void adicionar_feed(CLIENT *novo_cliente)
{
    // Adicionar cliente à lista de registrados
    pthread_mutex_lock(&clientes_mutex);
    novo_cliente->nextClient = users.clientes_registrados;
    users.clientes_registrados = novo_cliente;
    users.num_utilizadores++; // Incrementa o número de utilizadores
    pthread_mutex_unlock(&clientes_mutex);
}

// Função para imprimir todos os topicos
void print_topicos()
{
    pthread_mutex_lock(&topicos_mutex);
    printf("Lista de topicos:\n");
    for (int i = 0; i < topicos.num_topicos; i++)
    {
        printf("Topico: %s\n", topicos.topicos[i].nome);
    }
    pthread_mutex_unlock(&topicos_mutex);
}

// Escrever ficheiro txt
void writeFile()
{
    FILE *file;
    char *filename = getenv("MSG_FICH");
    if (filename == NULL)
    {
        printf("\nVariável de ambiente MSG_FICH não definida.\n");
        return;
    }

    printf("[DEBUG] - Nome do arquivo: %s\n", filename);

    file = fopen(filename, "w");

    if (file == NULL)
    {
        printf("\nNão foi possível abrir o arquivo.\n");
        return;
    }

    printf("[DEBUG] - Arquivo aberto com sucesso.\n");

    pthread_mutex_lock(&mensagens_mutex);
    for (int i = 0; i < mensagens.num_mensagens; i++)
    {
        printf("[DEBUG] - Escrevendo mensagem %d: %s %s %d %s\n", i, mensagens.mensagens[i].topico, mensagens.mensagens[i].username, mensagens.mensagens[i].duracao, mensagens.mensagens[i].mensagem);
        fprintf(file, "%s %s %d %s\n", mensagens.mensagens[i].topico, mensagens.mensagens[i].username, mensagens.mensagens[i].duracao, mensagens.mensagens[i].mensagem);
    }
    pthread_mutex_unlock(&mensagens_mutex);

    fclose(file);
    printf("Escrito com sucesso\n");
}

// Ler ficheiro txt
void readFile()
{
    FILE *file;
    char linha[400];
    int count = 0;

    char *filename = getenv("MSG_FICH");
    if (filename == NULL)
    {
        printf("\nVariável de ambiente MSG_FICH não definida.\n");
        return;
    }

    file = fopen(filename, "r");

    if (file == NULL)
    {
        printf("\nNão foi possível abrir o arquivo.\n");
        return;
    }

    // Ler o arquivo
    while (fgets(linha, sizeof(linha), file) != NULL)
    {
        linha[strcspn(linha, "\r\n")] = '\0';
        mensagem_t msg;
        if (sscanf(linha, "%29s %19s %d %299[^\n]", msg.topico, msg.username, &msg.duracao, msg.mensagem) == 4)
        {
            toUpperString(msg.topico);
            toUpperString(msg.username);
            int topico_existe = 0;

            pthread_mutex_lock(&topicos_mutex);
            for (int i = 0; i < topicos.num_topicos; i++)
            {
                if (strcmp(topicos.topicos[i].nome, msg.topico) == 0)
                {
                    topico_existe = 1;
                    break;
                }
            }
            pthread_mutex_unlock(&topicos_mutex);

            if (!topico_existe)
            {
                pthread_mutex_lock(&topicos_mutex);
                strcpy(topicos.topicos[topicos.num_topicos].nome, msg.topico);
                topicos.num_topicos++;
                pthread_mutex_unlock(&topicos_mutex);
            }

            pthread_mutex_lock(&mensagens_mutex);
            mensagens.mensagens[mensagens.num_mensagens] = msg;
            mensagens.num_mensagens++;
            pthread_mutex_unlock(&mensagens_mutex);
        }
        else
        {
            printf("Erro ao ler a linha: %s \n", linha);
        }
    }

    fclose(file);
    printf("Lido com sucesso\n");
}

// Função para imprimir todos os utilizadores registados
void print_users()
{
    pthread_mutex_lock(&clientes_mutex);
    CLIENT *atual = users.clientes_registrados;
    printf("Lista de utilizadores registados:\n");
    while (atual)
    {
        printf("Nome: %s, PID: %d\n", atual->user.nome, atual->PidRemetente);
        printf("TOPICOS SUBSCRITOS:\n");
        int i = atual->user.num_topicos;
        while (i > 0)
        {
            printf("Topico: %s\n", atual->user.topicos.topicos[i - 1].nome);
            i--;
        }
        atual = atual->nextClient;
    }
    pthread_mutex_unlock(&clientes_mutex);
}

// Função para Remover Utilizador
void Remover_Utilizador(char nome_utilizador[])
{
    int feed_fifo_fd;
    resposta_t resposta;
    if (verifica_nome(nome_utilizador))
    {
        CLIENT *current = users.clientes_registrados;
        while (current != NULL)
        { // Percorre enquanto houver clientes
            if (strcmp(current->user.nome, nome_utilizador) == 0)
            { // Comparação dos nomes
                printf("[MANAGER] utilizador Removido: %d \n", current->PidRemetente);
                strcpy(resposta.resposta, EXIT);
                // Abrir FIFO do cliente
                feed_fifo_fd = abre_ClientPipe(current->PidRemetente);
                if (feed_fifo_fd == -1)
                {
                    return;
                }

                // Enviar resposta para o cliente
                printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);

                if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
                {
                    perror("[ERRO] - Falha ao enviar resposta para o cliente");
                }

                return;
            }
            current = current->nextClient; // Avança para o próximo cliente
        }
        printf("[MANAGER] utilizador Removido: %s \n", nome_utilizador);
    }
    else
    {
        printf("Utilizador não registado\n");
    }
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

// Função para verificar se o utilizar subscreveu topico
int verificaSubscricao(pedido_t pedido)
{
    pthread_mutex_lock(&clientes_mutex);
    CLIENT *atual = users.clientes_registrados;
    while (atual)
    {
        if (atual->PidRemetente == pedido.PidRemetente)
        {
            for (int i = 0; i < atual->user.num_topicos; i++)
            {
                if (strcmp(atual->user.topicos.topicos[i].nome, pedido.mensagem.topico) == 0)
                {
                    pthread_mutex_unlock(&clientes_mutex);
                    return 1; // Tópico subscrito
                }
            }
            break;
        }
        atual = atual->nextClient;
    }
    pthread_mutex_unlock(&clientes_mutex);
    return 0; // topico nao subscrito
}

// Função para processar pedido do Manager
void processa_pedido(int manager_fifo_fd)
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
        // Verificar se o nome já está a ser usado
        if (verifica_nome(pedido.nome_user))
        {
            printf("[MANAGER] Cliente já registrado: %s. Ignorando pedido.\n", pedido.nome_user);
            strcpy(resposta.resposta, FEED_RECUSADO);

            // Abrir FIFO do cliente
            feed_fifo_fd = abre_ClientPipe(pedido.PidRemetente);
            if (feed_fifo_fd == -1)
            {
                free(newClient);
                return;
            }

            // Enviar resposta para o cliente
            printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);

            if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
            {
                perror("[ERRO] - Falha ao enviar resposta para o cliente");
            }

            return;
        }
        else if (users.num_utilizadores >= MAX_USERS)
        {
            printf("[MANAGER] Maximo de utilizadores atingido. Ignorando pedido.\n");
            strcpy(resposta.resposta, FEED_ESPERA);

            // Abrir FIFO do cliente
            feed_fifo_fd = abre_ClientPipe(pedido.PidRemetente);
            if (feed_fifo_fd == -1)
            {
                free(newClient);
                return;
            }

            // Enviar resposta para o cliente
            printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);

            if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
            {
                perror("[ERRO] - Falha ao enviar resposta para o cliente");
            }

            return;
        }

        // Alocar e registrar o cliente
        newClient = malloc(sizeof(CLIENT));
        if (!newClient)
        {
            fprintf(stderr, "[ERRO] - Falha ao alocar memória para o novo cliente.\n");
            return;
        }

        // Preencher dados do cliente
        pthread_mutex_lock(&clientes_mutex);
        newClient->PidRemetente = pedido.PidRemetente;
        strcpy(newClient->user.nome, pedido.nome_user);
        newClient->user.num_topicos = 0;
        newClient->nextClient = NULL;
        pthread_mutex_unlock(&clientes_mutex);

        printf("[MANAGER] Novo cliente registrado: %s\n", newClient->user.nome);

        // Adicionar feed à lista de registados
        adicionar_feed(newClient);

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
    }
    else if (strcmp(pedido.comando, TOPICS) == 0)
    {
        printf("[MANAGER] - Comando TOPICS recebido.\n");
        resposta.topicos = topicos;
        strcpy(resposta.resposta, TOPICS);

        // Abrir FIFO do cliente
        feed_fifo_fd = abre_ClientPipe(pedido.PidRemetente);
        if (feed_fifo_fd == -1)
        {
            perror("[ERRO] - Falha ao abrir FIFO do cliente");
            return;
        }

        // Enviar resposta para o cliente
        printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);
        if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
        {
            perror("[ERRO] - Falha ao enviar resposta para o cliente");
        }
    }
    else if (strcmp(pedido.comando, SUBSCRIBE) == 0)
    {
        printf("[MANAGER] - Comando SUBSCRIBE recebido.\n");

        if (verifica_topico(pedido.mensagem.topico))
        {
            if (verificaSubscricao(pedido))
            {
                strcpy(resposta.resposta, TOPICO_JA_SUBSCRITO);

                // Abrir FIFO do cliente
                feed_fifo_fd = abre_ClientPipe(pedido.PidRemetente);
                if (feed_fifo_fd == -1)
                {
                    free(newClient);
                    return;
                }

                // Enviar resposta para o cliente
                printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);

                if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
                {
                    perror("[ERRO] - Falha ao enviar resposta para o cliente");
                }
                return;
            }

            // se existir, adiciona o topico ao utilizador
            pthread_mutex_lock(&clientes_mutex);
            CLIENT *atual = users.clientes_registrados;
            while (atual)
            {
                if (atual->PidRemetente == pedido.PidRemetente)
                {
                    // Adicionar o tópico ao utilizador
                    strncpy(atual->user.topicos.topicos[atual->user.num_topicos].nome, pedido.mensagem.topico, TOPIC_LENGTH - 1);
                    atual->user.topicos.topicos[atual->user.num_topicos].nome[TOPIC_LENGTH - 1] = '\0';
                    atual->user.num_topicos++;
                    printf("[DEBUG] - Tópico adicionado ao utilizador: %d\n", atual->user.num_topicos);
                    break;
                }
                atual = atual->nextClient;
            }
            pthread_mutex_unlock(&clientes_mutex);

            printf("[MANAGER] - Tópico %s existe.\n", pedido.mensagem.topico);
            strncpy(resposta.resposta, TOPICO_SUBSCRITO, sizeof(resposta.resposta) - 1);
            resposta.resposta[sizeof(resposta.resposta) - 1] = '\0';

            // Abrir FIFO do cliente
            feed_fifo_fd = abre_ClientPipe(pedido.PidRemetente);
            if (feed_fifo_fd == -1)
            {
                perror("[ERRO] - Falha ao abrir FIFO do cliente");
                return;
            }

            // Enviar resposta de subscrição
            printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);
            if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
            {
                perror("[ERRO] - Falha ao enviar resposta para o cliente");
                close(feed_fifo_fd);
                return;
            }

            memset(&resposta, 0, sizeof(resposta));

            // Enviar mensagens persistentes do tópico
            pthread_mutex_lock(&mensagens_mutex);
            for (int i = 0; i < mensagens.num_mensagens; i++)
            {
                if (strcmp(mensagens.mensagens[i].topico, pedido.mensagem.topico) == 0)
                {
                    mensagem_t msg = mensagens.mensagens[i];

                    // Enviar mensagem persistente
                    snprintf(resposta.resposta, sizeof(resposta.resposta), "%s: %s",
                             mensagens.mensagens[i].topico, mensagens.mensagens[i].mensagem);

                    printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);
                    if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
                    {
                        perror("[ERRO] - Falha ao enviar mensagem persistente para o cliente");
                        break;
                    }
                }
            }
            pthread_mutex_unlock(&mensagens_mutex);

            close(feed_fifo_fd); // Fechar o FIFO
        }
        else
        {
            if (topicos.num_topicos >= MAX_TOPICS)
            {
                strcpy(resposta.resposta, TOPICO_MAXIMO);

                // Abrir FIFO do cliente
                feed_fifo_fd = abre_ClientPipe(pedido.PidRemetente);
                if (feed_fifo_fd == -1)
                {
                    free(newClient);
                    return;
                }

                // Enviar resposta para o cliente
                printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);

                if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
                {
                    perror("[ERRO] - Falha ao enviar resposta para o cliente");
                }

                return;
                printf("[MANAGER] - Maximo de topicos atingido.\n");
                return;
            }

            // cria o topico e associa ao cliente
            pthread_mutex_lock(&topicos_mutex);
            strcpy(topicos.topicos[topicos.num_topicos].nome, pedido.mensagem.topico);
            topicos.num_topicos++;
            topicos.topicos[topicos.num_topicos - 1].estado = DESBLOQUEADO;
            pthread_mutex_unlock(&topicos_mutex);

            pthread_mutex_lock(&clientes_mutex);
            CLIENT *atual = users.clientes_registrados;
            while (atual)
            {
                if (atual->PidRemetente == pedido.PidRemetente)
                {
                    strcpy(atual->user.topicos.topicos[atual->user.num_topicos].nome, pedido.mensagem.topico);
                    atual->user.num_topicos++;
                    break;
                }
                atual = atual->nextClient;
            }
            pthread_mutex_unlock(&clientes_mutex);

            strcpy(resposta.resposta, TOPICO_SUBSCRITO);

            // Abrir FIFO do cliente
            feed_fifo_fd = abre_ClientPipe(pedido.PidRemetente);
            if (feed_fifo_fd == -1)
            {
                free(newClient);
                return;
            }

            // Enviar resposta para o cliente
            printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);

            if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
            {
                perror("[ERRO] - Falha ao enviar resposta para o cliente");
            }
        }
        print_topicos();
    }
    else if (strcmp(pedido.comando, UNSUBSCRIBE) == 0)
    {
        printf("[MANAGER] - Comando UNSUBSCRIBE recebido.\n");

        if (verifica_topico(pedido.mensagem.topico))
        {
            // Tópico existe -> verificar se o cliente está subscrito
            int aux = 0;
            pthread_mutex_lock(&clientes_mutex);
            CLIENT *atual = users.clientes_registrados;
            while (atual)
            {
                if (atual->PidRemetente == pedido.PidRemetente)
                {
                    for (int i = 0; i < atual->user.num_topicos; i++)
                    {
                        if (strcmp(atual->user.topicos.topicos[i].nome, pedido.mensagem.topico) == 0)
                        {
                            aux = 1; // Tópico encontrado e será removido
                            for (int j = i; j < atual->user.num_topicos - 1; j++)
                            {
                                strcpy(atual->user.topicos.topicos[j].nome, atual->user.topicos.topicos[j + 1].nome);
                            }
                            atual->user.num_topicos--;
                            break;
                        }
                    }
                    break;
                }
                atual = atual->nextClient;
            }
            pthread_mutex_unlock(&clientes_mutex);

            if (aux == 0)
            {
                // Cliente não está subscrito no tópico
                strcpy(resposta.resposta, TOPICO_NAO_SUBSCRITO);

                // Abrir FIFO do cliente
                feed_fifo_fd = abre_ClientPipe(pedido.PidRemetente);
                if (feed_fifo_fd == -1)
                {
                    perror("[ERRO] - Falha ao abrir FIFO do cliente");
                    return;
                }

                // Enviar resposta para o cliente
                printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);
                if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
                {
                    perror("[ERRO] - Falha ao enviar resposta para o cliente");
                }
            }
            else
            {
                // Tópico existe mandar resposta ao cliente a dizer que foi apagado
                strcpy(resposta.resposta, TOPICO_UNSUBSCRIBE);

                // Abrir FIFO do cliente
                feed_fifo_fd = abre_ClientPipe(pedido.PidRemetente);
                if (feed_fifo_fd == -1)
                {
                    perror("[ERRO] - Falha ao abrir FIFO do cliente");
                    return;
                }

                // Enviar resposta para o cliente
                printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);
                if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
                {
                    perror("[ERRO] - Falha ao enviar resposta para o cliente");
                }

                // Verificar se outros utilizadores ainda possuem o tópico
                pthread_mutex_lock(&clientes_mutex);
                atual = users.clientes_registrados;
                int topico_usado = 0;
                while (atual)
                {
                    for (int i = 0; i < atual->user.num_topicos; i++)
                    {
                        if (strcmp(atual->user.topicos.topicos[i].nome, pedido.mensagem.topico) == 0)
                        {
                            topico_usado = 1; // Outro utilizador ainda possui o tópico
                            break;
                        }
                    }
                    if (topico_usado)
                    {
                        break;
                    }
                    atual = atual->nextClient;
                }
                pthread_mutex_unlock(&clientes_mutex);

                if (topico_usado)
                {
                    return;
                }

                // Verificar mensagens persistentes com o tópico
                pthread_mutex_lock(&mensagens_mutex);
                for (int i = 0; i < mensagens.num_mensagens; i++)
                {
                    if (strcmp(mensagens.mensagens[i].topico, pedido.mensagem.topico) == 0)
                    {
                        pthread_mutex_unlock(&mensagens_mutex);
                        return; // Há mensagens persistentes
                    }
                }
                pthread_mutex_unlock(&mensagens_mutex);

                // Remover o tópico da lista de tópicos globais
                pthread_mutex_lock(&topicos_mutex);
                for (int i = 0; i < topicos.num_topicos; i++)
                {
                    if (strcmp(topicos.topicos[i].nome, pedido.mensagem.topico) == 0)
                    {
                        for (int j = i; j < topicos.num_topicos - 1; j++)
                        {
                            strcpy(topicos.topicos[j].nome, topicos.topicos[j + 1].nome);
                        }
                        topicos.num_topicos--;
                        break;
                    }
                }
                pthread_mutex_unlock(&topicos_mutex);
            }
        }
        else
        {
            // Tópico não existe
            strcpy(resposta.resposta, TOPICO_NAO_EXISTE);

            // Abrir FIFO do cliente
            feed_fifo_fd = abre_ClientPipe(pedido.PidRemetente);
            if (feed_fifo_fd == -1)
            {
                perror("[ERRO] - Falha ao abrir FIFO do cliente");
                return;
            }

            // Enviar resposta para o cliente
            printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);
            if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
            {
                perror("[ERRO] - Falha ao enviar resposta para o cliente");
            }
        }
    }
    else if (strcmp(pedido.comando, MSG) == 0)
    {
        if (verificaSubscricao(pedido))
        {
            int bloqueado = 0;

            // Verificar se o tópico está bloqueado
            pthread_mutex_lock(&topicos_mutex);
            for (int i = 0; i < topicos.num_topicos; i++)
            {
                if (strcmp(topicos.topicos[i].nome, pedido.mensagem.topico) == 0)
                {
                    if (topicos.topicos[i].estado == BLOQUEADO)
                    {
                        bloqueado = 1;
                    }
                    break;
                }
            }
            pthread_mutex_unlock(&topicos_mutex);

            if (bloqueado)
            {
                strcpy(resposta.resposta, TOPICO_BLOQUEADO);
                feed_fifo_fd = abre_ClientPipe(pedido.PidRemetente);
                if (feed_fifo_fd != -1)
                {
                    printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);
                    if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
                    {
                        perror("[ERRO] - Falha ao enviar resposta para o cliente");
                    }
                }
                return;
            }

                        if (pedido.mensagem.duracao > 0)
            {

                int num_msg = 0;
                pthread_mutex_lock(&topicos_mutex);
                for (int i = 0; i < topicos.num_topicos; i++)
                {
                    if (strcmp(topicos.topicos[i].nome, pedido.mensagem.topico) == 0)
                    {
                        num_msg = topicos.topicos[i].num_mensagens;
                        break;
                    }
                }
                pthread_mutex_unlock(&topicos_mutex);

                pthread_mutex_lock(&mensagens_mutex);
                if (num_msg < MAX_TOPICS_MSG_PERSIS)
                {
                    strncpy(mensagens.mensagens[mensagens.num_mensagens].topico, pedido.mensagem.topico, TOPIC_LENGTH - 1);
                    mensagens.mensagens[mensagens.num_mensagens].topico[TOPIC_LENGTH - 1] = '\0';

                    strncpy(mensagens.mensagens[mensagens.num_mensagens].mensagem, pedido.mensagem.mensagem, TAM_MSG - 1);
                    mensagens.mensagens[mensagens.num_mensagens].mensagem[TAM_MSG - 1] = '\0';
                    strncpy(mensagens.mensagens[mensagens.num_mensagens].username, pedido.mensagem.username, TAM_NOME - 1);
                    mensagens.mensagens[mensagens.num_mensagens].duracao = pedido.mensagem.duracao;
                    mensagens.num_mensagens++;
                }
                pthread_mutex_unlock(&mensagens_mutex);

                // Atualizar o número de mensagens no tópico correspondente
                pthread_mutex_lock(&topicos_mutex);
                for (int i = 0; i < topicos.num_topicos; i++)
                {
                    if (strcmp(topicos.topicos[i].nome, pedido.mensagem.topico) == 0)
                    {
                        topicos.topicos[i].num_mensagens++;
                        break;
                    }
                }
                pthread_mutex_unlock(&topicos_mutex);
            }

            pthread_mutex_lock(&clientes_mutex);
            CLIENT *atual = users.clientes_registrados;
            while (atual)
            {
                for (int i = 0; i < atual->user.num_topicos; i++)
                {
                    if (strcmp(atual->user.topicos.topicos[i].nome, pedido.mensagem.topico) == 0 && atual->PidRemetente != pedido.PidRemetente)
                    {
                        feed_fifo_fd = abre_ClientPipe(atual->PidRemetente);
                        if (feed_fifo_fd == -1)
                        {
                            fprintf(stderr, "[ERRO] - Falha ao abrir FIFO do cliente (PID: %d)\n", atual->PidRemetente);
                            continue;
                        }

                        snprintf(resposta.resposta, sizeof(resposta.resposta), "%s para %s: %s", pedido.mensagem.username, pedido.mensagem.topico, pedido.mensagem.mensagem);

                        if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
                        {
                            perror("[ERRO] - Falha ao enviar mensagem para o cliente");
                        }
                        close(feed_fifo_fd); // Fecha o FIFO após o uso
                        break;
                    }
                }
                atual = atual->nextClient;
            }
            pthread_mutex_unlock(&clientes_mutex);
        }
        else
        {
            // Não tem subscrição no topico
            strcpy(resposta.resposta, TOPICO_NAO_SUBSCRITO);

            // Abrir FIFO do cliente
            feed_fifo_fd = abre_ClientPipe(pedido.PidRemetente);
            if (feed_fifo_fd == -1)
            {
                perror("[ERRO] - Falha ao abrir FIFO do cliente");
                return;
            }

            // Enviar resposta para o cliente
            printf("\n[MANAGER] Enviando resposta: %s\n", resposta.resposta);
            if (write(feed_fifo_fd, &resposta, sizeof(resposta)) == -1)
            {
                perror("[ERRO] - Falha ao enviar resposta para o cliente");
            }
        }
    }
    else if (strcmp(pedido.comando, EXIT) == 0)
    {
        printf("[MANAGER] - Comando EXIT recebido.\n");

        pthread_mutex_lock(&clientes_mutex);
        CLIENT *atual = users.clientes_registrados;
        CLIENT *anterior = NULL;
        while (atual)
        {

            if (atual->PidRemetente == pedido.PidRemetente)
            {
                if (anterior)
                {
                    anterior->nextClient = atual->nextClient;
                }
                else
                {
                    users.clientes_registrados = atual->nextClient;
                }

                free(atual);
                users.num_utilizadores--;
                pthread_mutex_unlock(&clientes_mutex);
                return;
            }
            anterior = atual;
            atual = atual->nextClient;
        }

        pthread_mutex_unlock(&clientes_mutex);
    }
    else
    {
        printf("\n[ERRO] - Comando inválido: %s\n", pedido.comando);
    }

    print_users();
}

// Função para verificar comandos
void VerificaComando(char *str)
{
    toUpperString(str);

    // Verifica se é o comando CLOSE
    if (strcmp(str, "CLOSE") == 0)
    {
        printf("\n[MANAGER] Comando recebido: desligar.\n");
        sair();

        // Verifica se é o comando USERS
    }
    else if (strcmp(str, "USERS") == 0)
    {
        print_users();

        // Verifica se é o comando TOPICS
    }
    else if (strcmp(str, "TOPICS") == 0)
    {
        print_topicos();

        // Verifica se é o comando REMOVE
    }
    else if (strncmp(str, "REMOVE ", 7) == 0 || strcmp(str, "REMOVE") == 0)
    {
        int i = 7;
        int encontrado = 0;
        int contador = 0;
        int len = strlen(str);
        char remove_str[50] = {0}; // Inicializa com zeros

        // Captura o utilizador
        while (i < len && str[i] != ' ' && contador < sizeof(remove_str) - 1)
        {
            remove_str[contador++] = str[i++];
        }
        remove_str[contador] = '\0';

        if (contador > 0)
            encontrado += 1;

        if (encontrado == 1)
        {
            Remover_Utilizador(remove_str);
        }
        else
        {
            printf("Insira no formato: remove <username>\n");
        }

        // Verifica se é o comando SHOW
    }
    else if ((strncmp(str, "SHOW ", 5) == 0) || strcmp(str, "SHOW") == 0)
    {
        int i = 5;
        int encontrado = 0;
        int contador = 0;
        int len = strlen(str);
        char topico_str[50] = {0}; // Inicializa com zeros

        // Captura o topico
        while (i < len && str[i] != ' ' && contador < sizeof(topico_str) - 1)
        {
            topico_str[contador++] = str[i++];
        }
        topico_str[contador] = '\0';

        if (contador > 0)
        {
            encontrado += 1;
        }

        if (encontrado == 1)
        {
            printf("A mostrar topico %s ...\n", topico_str);
        }
        else
        {
            printf("Insira no formato: show <topico>\n");
        }

        toUpperString(topico_str);
        Mostrar_MensagensTopico(topico_str);
    }
    else if ((strncmp(str, "LOCK ", 5) == 0) || strcmp(str, "LOCK") == 0)
    {
        int i = 5;
        int encontrado = 0;
        int contador = 0;
        int len = strlen(str);
        char topico_str[50] = {0}; // Inicializa com zeros

        // Captura o topico
        while (i < len && str[i] != ' ' && contador < sizeof(topico_str) - 1)
        {
            topico_str[contador++] = str[i++];
        }
        topico_str[contador] = '\0';

        if (contador > 0)
            encontrado += 1;

        if (encontrado != 1)
        {
            printf("Insira no formato: lock <topico>\n");
            return;
        }

        printf("A bloquear topico %s ...\n", topico_str);

        toUpperString(topico_str);

        if (verifica_topico(topico_str) == 0)
        {
            printf("[INFO] - Topico não existe\n");
            return;
        }

        int bloqueado_sucesso = 0;
        pthread_mutex_lock(&topicos_mutex);
        for (int i = 0; i < topicos.num_topicos; i++)
        {
            if (strcmp(topico_str, topicos.topicos[i].nome) == 0)
            {
                topicos.topicos[i].estado = BLOQUEADO;
                bloqueado_sucesso = 1; // Marca como sucesso
                break;
            }
        }
        pthread_mutex_unlock(&topicos_mutex);

        if (bloqueado_sucesso)
        {
            printf("[MANAGER] - Tópico '%s' bloqueado com sucesso.\n", topico_str);
        }
        else
        {
            printf("[MANAGER] - Falha ao bloquear o tópico '%s'. Tópico não encontrado.\n", topico_str);
        }
    }
    else if ((strncmp(str, "UNLOCK ", 7) == 0) || strcmp(str, "UNLOCK") == 0)
    {
        int i = 7;
        int encontrado = 0;
        int contador = 0;
        int len = strlen(str);
        char topico_str[50] = {0}; // Inicializa com zeros

        // Captura o topico
        while (i < len && str[i] != ' ' && contador < sizeof(topico_str) - 1)
        {
            topico_str[contador++] = str[i++];
        }
        topico_str[contador] = '\0';

        if (contador > 0)
        {
            encontrado += 1;
        }

        if (encontrado != 1)
        {
            printf("Insira no formato: lock <topico>\n");
            return;
        }

        printf("A bloquear topico %s ...\n", topico_str);

        toUpperString(topico_str);

        if (verifica_topico(topico_str) == 0)
        {
            printf("[INFO] - Topico não existe\n");
            return;
        }

        int desbloqueado = 0;
        pthread_mutex_lock(&topicos_mutex);
        for (int i = 0; i < topicos.num_topicos; i++)
        {
            if (strcmp(topico_str, topicos.topicos[i].nome) == 0)
            {
                topicos.topicos[i].estado = DESBLOQUEADO;
                desbloqueado = 1; // Marca como sucesso
                break;
            }
        }
        pthread_mutex_unlock(&topicos_mutex);

        if (desbloqueado)
        {
            printf("[MANAGER] - Tópico '%s' desbloqueado com sucesso.\n", topico_str);
        }
        else
        {
            printf("[MANAGER] - Falha ao desbloquear o tópico '%s'. Tópico não encontrado.\n", topico_str);
        }
    }

    // Comando não reconhecido
    else
    {
        printf("Comando não reconhecido: %s\n", str);
    }
}

void *thread_contador(void *arg)
{
    ThreadControl *control = (ThreadControl *)arg;

    while (control->running)
    {
        sleep(1);

        // verificar se o tempo da mensagem chegou ao fim, se chegou remover a mensagem
        pthread_mutex_lock(&mensagens_mutex);
        for (int i = 0; i < mensagens.num_mensagens; i++)
        {
            if (mensagens.mensagens[i].duracao > 0)
            {
                mensagens.mensagens[i].duracao--;
                printf("[DEBUG] - Mensagem '%s' no tópico '%s' com duração %d.\n", mensagens.mensagens[i].mensagem, mensagens.mensagens[i].topico, mensagens.mensagens[i].duracao);
                if (mensagens.mensagens[i].duracao == 0)
                {
                    printf("[THREAD] - Mensagem '%s' no tópico '%s' expirada e removida.\n", mensagens.mensagens[i].mensagem, mensagens.mensagens[i].topico);
                    for (int j = i; j < mensagens.num_mensagens - 1; j++)
                    {
                        mensagens.mensagens[j] = mensagens.mensagens[j + 1];
                    }
                    mensagens.num_mensagens--;
                    i--; // Ajustar índice após remoção
                }
            }
        }
        pthread_mutex_unlock(&mensagens_mutex);
    }

    return NULL;
}

// Função principal
int main(int argc, char *argv[])
{
    int continua = 1;
    fd_set fdset, fdset_backup;
    struct sigaction sa;
    pthread_t thread;
    ThreadControl control;

    pthread_mutex_init(&clientes_mutex, NULL);
    pthread_mutex_init(&mensagens_mutex, NULL);
    pthread_mutex_init(&topicos_mutex, NULL);

    // Inicializar listas
    users.clientes_registrados = NULL;
    users.num_utilizadores = 0;
    topicos.num_topicos = 0;

    control.running = 1;

    // Ler ficheiro de mensagens
    readFile();

    // cria thread de tempo
    if (pthread_create(&thread, NULL, thread_contador, &control) != 0)
    {
        perror("[ERRO] - Falha ao criar a thread de tempo");
        return EXIT_FAILURE;
    }

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

    while (CONTROLADOR)
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
                if (len > 0 && entrada[len - 1] == '\n')
                {
                    entrada[len - 1] = '\0';
                }

                VerificaComando(entrada);
            }
        }

        if (FD_ISSET(manager_fifo_fd, &fdset_backup))
        {
            processa_pedido(manager_fifo_fd);
        }
    }

    writeFile();
    control.running = 0;
    pthread_join(thread, NULL);
    printf("\n[MANAGER] - Sistema encerrado.\n");
    return 0;
}