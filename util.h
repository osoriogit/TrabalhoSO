#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>


#define MAX_USERS 10
#define MAX_USERNAME_SIZE 30
#define MAX_TOPICS 20
#define MAX_PERSISTENT_MESSAGES 5
#define MAX_TOPIC_NAME 20
#define MAX_MESSAGE_BODY 300
#define FIFO_SRV "fifo_srv"

typedef struct {
    char username[30];
    char acao[15];
    char topico[MAX_TOPIC_NAME];
    int duracao;
    char mensagem[MAX_MESSAGE_BODY];
} Pedido;

typedef struct {
    char motivo[MAX_MESSAGE_BODY];
    char mensagem[MAX_MESSAGE_BODY];
} Resposta;

typedef struct {
    char nome[MAX_TOPIC_NAME];
    char persistentMsg[MAX_PERSISTENT_MESSAGES][MAX_MESSAGE_BODY];
    int nPersistent;
    int isBlocked;
} Topics;

typedef struct{
    char username[MAX_USERNAME_SIZE];
    char subs[MAX_TOPICS][MAX_TOPIC_NAME];
    int nSubs;
} Users;
