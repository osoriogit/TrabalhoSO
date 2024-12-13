//
// ----------------CLIENT----------------
//
#include "util.h"
#define FIFO_MAN "server"

// Função para enviar mensagem ao servidor
void sendMessage(int fd, Pedido *p, const char *cmd) {
    if (write(fd, p, sizeof(Pedido)) == -1) {
        printf("[ERRO] Falha ao enviar mensagem para o servidor: %s\n", cmd);
    }
}

// Função para configurar a estrutura Pedido
void configMessage(Pedido *p, const char *acao, const char *topico, int duracao, const char *mensagem) {
    strncpy(p->acao, acao, sizeof(p->acao) - 1);
    strncpy(p->topico, topico, sizeof(p->topico) - 1);
    p->duracao = duracao;
    strncpy(p->mensagem, mensagem, sizeof(p->mensagem) - 1);
}

int main(int argc, char *argv[]) {
    char acao[15], topico[MAX_TOPIC_NAME], mensagem[MAX_MESSAGE_BODY];
    char cmd[600];
    int duracao;
    int fd_cli, fd;
    char username[MAX_USERNAME_SIZE];
    Pedido p;
    Resposta r;

    if (argc != 2) {
        printf("[ERRO] Sintaxe incorreta. Uso: <programa> <username>\n");
        exit(1);
    }

    strncpy(username, argv[1], sizeof(username) - 1);
    username[sizeof(username) - 1] = '\0'; // Garantir terminação segura

    // Verifica se o pipe do servidor existe.
    if (access(FIFO_MAN, F_OK) != 0) {
        printf("[ERRO] O servidor não está ligado\n");
        exit(2);
    }

    // Abertura do FIFO do servidor
    fd = open(FIFO_MAN, O_WRONLY);
    if (fd == -1) {
        printf("[ERRO] Não foi possível abrir o FIFO do servidor\n");
        exit(3);
    }
//login bellow

    // Enviar username ao servidor para validação.
    char username_pid[256];
    snprintf(username_pid, sizeof(username_pid), "%s_%d", username, getpid());

    // Criar FIFO temporário para validação
    if (mkfifo(username_pid, 0666) == -1) {
        printf("[ERRO] Falha ao criar o FIFO temporário\n");
        close(fd);
        exit(4);
    }

    // Abrir FIFO temporário
    int fd_cli_temp = open(username_pid, O_RDWR);
    if (fd_cli_temp == -1) {
        printf("[ERRO] Não foi possível abrir o FIFO temporário\n");
        close(fd);
        unlink(username_pid);
        exit(5);
    }

    //envia para validacao o username
    strncpy(p.username, username_pid, sizeof(p.username) - 1);
    configMessage(&p, "validar", "", -1, "");
    sendMessage(fd, &p, username_pid);

    printf("Banana1");
    // Receber resposta do servidor
    if (read(fd_cli_temp, &r, sizeof(Resposta)) != sizeof(Resposta)) {
        printf("[ERRO] Falha ao receber resposta do servidor\n");
        close(fd);
        close(fd_cli_temp);
        unlink(username_pid);
        exit(6);
    }
    printf("Banana2");
    // Validar a disponibilidade de conexão/username
    if (strcmp(r.motivo, "USER_ALR_EXISTS") == 0) {
        printf("[ERRO] Username inválido!\n");
        close(fd);
        close(fd_cli_temp);
        unlink(username_pid);
        exit(7);
    } else if (strcmp(r.motivo, "USER_LIMIT") == 0) {
        printf("[ERRO] Nº de utilizadores excedido, tente mais tarde!\n");
        close(fd);
        close(fd_cli_temp);
        unlink(username_pid);
        exit(8);
    } else {
        printf("[INFO] Bem-vindo, %s!\n", username);
    }

    // Fecha o FIFO temporário
    close(fd_cli_temp);
    unlink(username_pid);
//login above

    // Criar FIFO permanente para o cliente
    if (mkfifo(username, 0666) == -1) {
        printf("[ERRO] Falha ao criar o FIFO do cliente\n");
        close(fd);
        exit(9);
    }

    // Abrir FIFO permanente do cliente
    fd_cli = open(username, O_RDWR);
    if (fd_cli == -1) {
        printf("[ERRO] Não foi possível abrir o FIFO do cliente\n");
        unlink(username);
        close(fd);
        exit(10);
    }

    // Inicializa o username
    strncpy(p.username, username, sizeof(p.username) - 1);

    // Exibir menu
    //menuTable();

    fd_set read_fds;
    int max_fd = (fd_cli > STDIN_FILENO ? fd_cli : STDIN_FILENO) + 1; // Determinar maior descritor

    while (1) {
        FD_ZERO(&read_fds);          // Limpar conjunto de descritores
        FD_SET(STDIN_FILENO, &read_fds); // Adicionar entrada padrão (teclado)
        FD_SET(fd_cli, &read_fds);   // Adicionar FIFO cliente

        printf("CMD> ");
        fflush(stdout);

        if (select(max_fd, &read_fds, NULL, NULL, NULL) == -1) {
            perror("[ERRO] Erro no select");
            break;
        }

        // Verificar se há entrada no teclado
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (!fgets(cmd, sizeof(cmd), stdin)) {
                printf("[ERRO] Erro ao ler o comando. Tente novamente!\n");
                continue;
            }

            cmd[strcspn(cmd, "\n")] = '\0'; // Remover newline
            sscanf(cmd, "%s", acao);

            if (strcmp(acao, "exit") == 0) {
                if (strcmp(cmd, "exit") == 0) {
                    // Enviar notificação de desconexão antes de sair
                    configMessage(&p, "exit", "", -1, "");
                    sendMessage(fd, &p, "exit");

                    // Esperar a resposta do servidor para confirmação
                    Resposta res;
                    if (read(fd_cli, &res, sizeof(Resposta)) > 0) {
                        if (strcmp(res.motivo, "USER_LEAVE") == 0) {
                            printf("%s\n", res.mensagem);
                            break;
                        }
                    }
                    printf("[ERRO] Falha ao desconectar\n");
                } else {
                    printf("[ERRO] Sintaxe incorreta para o comando 'exit'\n");
                }
            } else if (strcmp(acao, "msg") == 0) {
                if (sscanf(cmd, "msg %s %d %[^\n]", topico, &duracao, mensagem) == 3) {
                    configMessage(&p, acao, topico, duracao, mensagem);
                    sendMessage(fd, &p, cmd);
                } else {
                    printf("[ERRO] Sintaxe incorreta para o comando 'msg <topico> <duracao> <mensagem>'\n");
                }
            } else if (strcmp(acao, "subscribe") == 0) {
                if (sscanf(cmd, "subscribe %s", topico) == 1) {
                    configMessage(&p, acao, topico, -1, "");
                    sendMessage(fd, &p, cmd);
                } else {
                    printf("[ERRO] Sintaxe incorreta para o comando 'subscribe <topico>'\n");
                }
            } else if (strcmp(acao, "unsubscribe") == 0) {
                if (sscanf(cmd, "unsubscribe %s", topico) == 1) {
                    configMessage(&p, acao, topico, -1, "");
                    sendMessage(fd, &p, cmd);
                } else {
                    printf("[ERRO] Sintaxe incorreta para o comando 'unsubscribe <topico>'\n");
                }
            } else if (strcmp(acao, "topics") == 0) {
                if (strcmp(cmd, "topics") == 0) {
                    // Mostra os topicos existentes
                    configMessage(&p, acao, "", -1, "");
                    sendMessage(fd, &p, cmd);
                } else {
                    printf("[ERRO] Sintaxe incorreta para o comando 'topics'\n");
                }
            } else {
                printf("[ERRO] Comando desconhecido\n");
            }
        }

        // Verificar se há mensagens do servidor
        if (FD_ISSET(fd_cli, &read_fds)) {
            Resposta res;
            if (read(fd_cli, &res, sizeof(Resposta)) > 0) {
                if (strcmp(res.motivo, "END_SERVER") == 0) {
                    printf("\n%s\n", res.mensagem);
                    printf("[INFO] Conexão com o servidor foi encerrada.\n");
                    break;
                } else if (strcmp(res.motivo, "USER_REMOVED") == 0) {
                    printf("\n%s\n", res.mensagem);
                    break;
                } else if (strcmp(res.motivo, "USER_LEFT") == 0) {
                    printf("\n%s\n", res.mensagem);
                } else {
                    printf("\n%s\n", res.mensagem);
                }
            } else {
                printf("[ERRO] Conexão com o servidor foi encerrada de forma inesperada.\n");
                break;
            }
        }
    }

    // Encerrar
    close(fd);
    close(fd_cli);
    unlink(username);
    printf("[INFO] Cliente terminado.\n");
    return 0;
}