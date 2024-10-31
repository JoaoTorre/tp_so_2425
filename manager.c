#include "manager.h"

void handle_signal(int sig, siginfo_t *info, void *ucontext)
{
    // chamar funcao de saida
}

int main(int argc, char *argv[])
{

    pthread_mutex_t trinco; // só pode ter um
    struct sigaction sa;

    pthread_mutex_init(&trinco, NULL);

    // lidar com sinais
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handle_signal;
    sigaction(SIGINT, &sa, NULL);

    return 0;
}