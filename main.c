#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "timer.h"

int TAMANHO_DO_BUFFER; //corresponde ao número de entradas do buffer (número de blocos do buffer)
int TAMANHO_DO_BLOCO; //corresponde ao número de inteiros armazenados em cada bloco
long long int quantidade_de_inteiros_totais; //armazena a quantidade de inteiros presentes no arquivo
long long int quantidade_de_blocos_totais; //corresponde à variável que armazenará a quantidade de blocos que serão utilizados no total no decorrer da execução
long long int vezes_de_acesso_ao_buffer; //corresponde ao número de vezes que o buffer será utilizado
long long int quantidade_de_blocos_no_ultimo_buffer; //corresponde à quantidade de blocos presentes no último acesso ao buffer
long long int quantidade_de_inteiros_no_ultimo_bloco; //corresponde à variável auxiliar para caso a divisão entre o número de inteiros do aquivo e tamanho do bloco não seja exata

int **buffer; //declaração do buffer que será usado
int *status; //vetor que indica quais blocos do buffer podem ser atualizados

FILE *fptr; //declaração da variável ponteiro que aponta para o arquivo binário

pthread_mutex_t lock_status; //variável lock para evitar condição de corrida ao acessar conteúdo do vetor status

pthread_cond_t cond_renova; //variável de bloqueio por condição para gerenciar a renovação dos blocos do buffer

//variáveis de gerencia da barreira
int contador = 4; //contador de threads que faltam chegar na barreira (possui valor 4 pois inclui a main, além das outras 3 threads)
pthread_mutex_t lock_barreira;
pthread_cond_t cond_bar;

//declarações de protótipos de funções
void barreira();
void * acha_maior_sequencia();
void * conta_sequencias_de_3();
void * conta_sequencias_de_0_a_5();

int main(int argc, char *argv[]) {

    double ini, fim; //variáveis de tomada de tempo
    GET_TIME(ini);

    /*
     * Bloco que recebe, avalia e processa os argumentos passados na linha de comando
     */

    if (argc == 4) { //Se havia o número correto de argumentos, trabalhamos com eles

        char *path = argv[1];
        fptr = fopen(path, "rb"); //abre o arquivo binário no modo leitura
        if (fptr == NULL) {

            fprintf(stderr, "Não foi possível localizar o arquivo.");
            return 1;
        }
        TAMANHO_DO_BUFFER = atoi(argv[2]); //armazena o tamanho do buffer escolhido pelo usuário
        TAMANHO_DO_BLOCO = atoi(argv[3]); //armazena o tamanho dos blocos escolhido pelo usuário
        fread(&quantidade_de_inteiros_totais, sizeof(int), 1,
              fptr); //realiza a leitura da quantidade de inteiros contidos no arquivo

        //calcula a quantidade de blocos totais que serão utilizados e o número de inteiros que haverá no último bloco
        if(quantidade_de_inteiros_totais % TAMANHO_DO_BLOCO == 0) {

            quantidade_de_blocos_totais = quantidade_de_inteiros_totais / TAMANHO_DO_BLOCO;
            quantidade_de_inteiros_no_ultimo_bloco = TAMANHO_DO_BLOCO;
        }
        else {

            quantidade_de_blocos_totais = (quantidade_de_inteiros_totais / TAMANHO_DO_BLOCO) + 1;
            quantidade_de_inteiros_no_ultimo_bloco = quantidade_de_inteiros_totais % TAMANHO_DO_BLOCO;
        }

        //calcula a quantidade de vezes que o buffer será acessado (para atualização dos blocos, assim como para leitura) e também calcula a quantidade de blocos no último acesso ao buffer
        if(quantidade_de_blocos_totais % TAMANHO_DO_BUFFER == 0) {

            vezes_de_acesso_ao_buffer = quantidade_de_blocos_totais / TAMANHO_DO_BUFFER;
            quantidade_de_blocos_no_ultimo_buffer = TAMANHO_DO_BUFFER;
        }
        else {

            vezes_de_acesso_ao_buffer = (quantidade_de_blocos_totais / TAMANHO_DO_BUFFER) + 1;
            quantidade_de_blocos_no_ultimo_buffer = quantidade_de_blocos_totais % TAMANHO_DO_BUFFER;
        }

    } else { //trata o caso onde argumentos são passados de forma indevida para a main

        fprintf(stderr, "Argumentos passados de forma indevida na função main.");
        return 1;
    }

    if (quantidade_de_inteiros_totais <
        TAMANHO_DO_BLOCO * TAMANHO_DO_BUFFER) { //trata um caso de incoerência nas entradas

        fprintf(stderr, "Não é coerente haver menos inteiros para processar do que o tamanho total do buffer.");
    }

    /*
     * Inicia o algoritmo principal do programa.
     */

    // bloco que aloca espaços de memória que serão necessários
    buffer = malloc(sizeof(int*) * TAMANHO_DO_BUFFER);
    for(int i = 0; i < TAMANHO_DO_BUFFER; i++) {

        buffer[i] = malloc(sizeof(int) * TAMANHO_DO_BLOCO);
    }
    status = (int *) malloc(TAMANHO_DO_BUFFER * sizeof(int)); //aloca memória para o vetor status
    for(int i = 0; i < TAMANHO_DO_BUFFER; i++) { //percorre o vetor status e sinaliza que todos os blocos devem ser atualizados

        status[i] = 0; //sinaliza que nenhuma thread está trabalhando nesse bloco
    }

    // bloquinho que inicializa as estruturas auxiliares
    /*
     * TODO
     */
    pthread_mutex_init(&lock_status, NULL);
    pthread_mutex_init(&lock_barreira, NULL);
    pthread_cond_init(&cond_renova, NULL);
    pthread_cond_init(&cond_bar, NULL);

    pthread_t tid[3]; //vetor de identificadores das threads

    // bloquinho que cria as threads que trabalharão com o buffer do arquivo
    if(pthread_create(&tid[0], NULL, acha_maior_sequencia, NULL)) {

        fprintf(stderr, "Erro ao criar uma thread.\n");
        return 1;
    }
    if(pthread_create(&tid[1], NULL, conta_sequencias_de_3, NULL)) {

        fprintf(stderr, "Erro ao criar uma thread.\n");
        return 1;
    }
    if(pthread_create(&tid[2], NULL, conta_sequencias_de_0_a_5, NULL)) {

        fprintf(stderr, "Erro ao criar uma thread.\n");
        return 1;
    }

    /*
     * Bloco na main que gerencia a renovação do conteúdo dos blocos do buffer
     */

    for(int i = 0; i < vezes_de_acesso_ao_buffer; i++) { //loop que atualiza o buffer na quantidade de vezes necessária

        long long int quantidade_de_blocos;
        if(i == vezes_de_acesso_ao_buffer - 1) { //se for o último acesso ao buffer, certifica-se de percorrer o número correto de blocos

            quantidade_de_blocos = quantidade_de_blocos_no_ultimo_buffer;
        }
        else {

            quantidade_de_blocos = TAMANHO_DO_BUFFER;
        }
        for(long long int j = 0; j < quantidade_de_blocos; j++) { //loop que atualiza a quantidade de blocos necessários

            long long int quantidade_de_inteiros;
            if(i == vezes_de_acesso_ao_buffer - 1 && j == quantidade_de_blocos - 1) {
                //se for a última vez que atualiza o buffer e for o último bloco que será atualizado, certifica-se de que lerá a quantidade correta de inteiros
                quantidade_de_inteiros = quantidade_de_inteiros_no_ultimo_bloco;
            }
            else {

                quantidade_de_inteiros = TAMANHO_DO_BUFFER;
            }

            pthread_mutex_lock(&lock_status);
            if(status[j] > 0) { //usamos if pois achamos que não há como essa condição voltar a ser verdadeira depois do unlock

                pthread_cond_wait(&cond_renova, &lock_status);
            }
            pthread_mutex_unlock(&lock_status);

            fread(buffer[j], sizeof(int), quantidade_de_inteiros, fptr);
            pthread_mutex_lock(&lock_status);
            status[j] = 3; //sinaliza no vetor global de controle que o bloco está disponível
            if(j == quantidade_de_blocos - 1) {

                barreira(); //se foi o último bloco a ser atualizada, chega na barreira para liberar todas as outras threads
            }
        }
    }

    /*
     * Trecho que aguarda o retorno das threads
     */

    long long int resposta1[3];
    int *resposta2;
    int *resposta3;
    pthread_join(tid[0], (void **) resposta1);
    pthread_join(tid[1], (void **) &resposta2);
    pthread_join(tid[2], (void **) &resposta3);

    // bloquinho que libera os locks e variáveis de condição
    pthread_mutex_destroy(&lock_status);
    pthread_mutex_destroy(&lock_barreira);
    pthread_cond_destroy(&cond_renova);
    pthread_cond_destroy(cond_bar);

    fclose(fptr); //fecha o arquivo que não será mais necessário

    // Encerra a tomada de tempo
    GET_TIME(fim);

    /*
     * Trecho que exibe a saída do programa
     */

    printf("Tempo de execução: %lf\n\n", fim - ini);

    if(resposta1[0] == -1) { //se retornou -1 significa que não encontrou nenhuma sequência

        printf("Maior sequência de valores idênticos: Nenhuma foi encontrada\n");
    }
    else {

        printf("Maior sequência de valores idênticos: %lld %lld %lld", resposta1[0], resposta1[1], resposta1[2]);
    }
    if(resposta2 == 0) {

        printf("Quantidade de triplas: Nenhuma foi encontrada\n");
    }
    else {

        printf("Quantidade de triplas: %d", *resposta2);
    }
    if(resposta3 == 0) {

        printf("Quantidade de ocorrências da sequência <012345>: Nenhuma encontrada\n");
    }
    else {

        printf("Quantidade de ocorrências da sequência <012345>: %d", *resposta3);
    }

    return 0;

}

void * acha_maior_sequencia() {

    long long int indiceInicialD; //variável que armazenará o índice inicial da maior sequência encontrada.
    long long int blocoInicialD; //variável que armazenará o bloco onde a maior sequência encontrada se inicia, com propósito de cálculo do índice inicial da mesma, que é feito no final da função
    int tamanhoD; //variável que armazenará o tamanho da maior sequência encontrada
    int valorD; //variável que armazenará o valor repetido na maior sequência encontrada
    // variáveis declaradas abaixo são auxiliares para armazenarem os dados da sequência que está sendo analisada no momento
    long long int indiceInicial;
    long long int blocoInicial;
    int tamanho;
    int valor;

    int primeiraVez = 1; //variável auxiliadora para saber se é a primeira vez que executamos o algoritmo

    for(long long int i = 0; i < quantidade_de_blocos_totais; i++) { //laço que se repetirá na quantidade de blocos que serão lidos

        int *bloco; //ponteiro para inteiro que apontará para o bloco da iteração vigente
        long long int tamanho_laco;
        if (i == quantidade_de_blocos_totais - 1) {

            tamanho_laco = quantidade_de_inteiros_no_ultimo_bloco;
        } else {

            tamanho_laco = TAMANHO_DO_BLOCO;
        }
        bloco = buffer[i % TAMANHO_DO_BUFFER]; //busca o bloco no buffer
        for (int j = 0; j < tamanho_laco; j++) { //percorre o bloco e executa o algoritmo

            if (primeiraVez) {

                valor = bloco[j];
                blocoInicial = i;
                indiceInicial = j;
                tamanho = 0;
                valorD = bloco[j];
                blocoInicialD = i;
                indiceInicialD = j;
                tamanhoD = 0;
                primeiraVez = 0; //atualiza a variável para sinalizar que já executou pela primeira vez
            } else {

                if (bloco[j] == valor) { //se for igual, incrementa o tamanho da sequência atual

                    tamanho++;
                } else { //se quebrou a sequência

                    if (tamanho >
                        tamanhoD) { //verifica se o tamanho da sequência que terminou é maior do que a registrada anteriormente
                        //se for, atualiza as informações da sequência definitiva
                        valorD = valor;
                        blocoInicialD = blocoInicial;
                        indiceInicialD = indiceInicial;
                        tamanhoD = tamanho;
                    }
                    // inicia a nova verificação de sequência
                    valor = bloco[j];
                    blocoInicial = i;
                    indiceInicial = j;
                    tamanho = 0;
                }
            }
        }

        /*
         * Sinaliza que terminou execução nesse bloco
         */
        pthread_mutex_lock(&lock_status);
        status[i % TAMANHO_DO_BUFFER]--;
        if (status[i % TAMANHO_DO_BUFFER] == 0) {

            pthread_cond_signal(
                    &cond_renova); //emite signal para desbloquear gerenciamento do buffer que ocorre na main :)
        }
        pthread_mutex_unlock(&lock_status);
        if (i != quantidade_de_blocos_totais - 1 && i != 0 && i % TAMANHO_DO_BUFFER == 0) {
            //se não for a última iteração no buffer e se for o último bloco do buffer, chegou na barreira
            barreira();
        }
    }
    if(tamanhoD == 0) { //se o tamanho definitivo for 0, significa que não encontrou nenhuma sequência

        int resultado[3] = {-1, -1, -1};
        pthread_exit((void *) resultado); //retorna vetor preenchido com -1 para sinalizar isso
    }
    else { //se encontrou alguma sequência

        indiceInicialD = (blocoInicialD * TAMANHO_DO_BLOCO) + indiceInicialD; //calcula o índice inicial dela
        long long int resultado[3] = {indiceInicialD, tamanhoD, valorD}; //cria o vetor de resultados (índice, tamanho e valor)
        pthread_exit((void *) resultado); //retorna o vetor
    }
}

void * conta_sequencias_de_3() {

    int *qntd3 = 0; //armazena a quantidade de ocorrências de sequências contínuas de tamanho 3
    int contador_tripla = 0; //variável auxiliar para checar se a sequência completa uma tripla
    int valor; //variável auxiliar para armazenar o valor das possíveis sequências que estão sendo avaliadas

    for(long long int i = 0; i < quantidade_de_blocos_totais; i++) { //laço que se repetirá na quantidade de blocos que serão lidos

        int *bloco; //ponteiro para inteiro que apontará para o bloco da iteração vigente
        long long int tamanho_laco;
        if (i == quantidade_de_blocos_totais - 1) {

            tamanho_laco = quantidade_de_inteiros_no_ultimo_bloco;
        } else {

            tamanho_laco = TAMANHO_DO_BLOCO;
        }
        bloco = buffer[i % TAMANHO_DO_BUFFER]; //busca o bloco no buffer

        for (int j = 0; j < tamanho_laco; j++) { //percorre o bloco e executa o algoritmo

            if (contador_tripla == 0) {

                valor = bloco[j];
                contador_tripla++;
            } else {

                if (bloco[j] == valor) {

                    contador_tripla++;
                    if (contador_tripla == 3) {

                        *qntd3++;
                        contador_tripla = 0;
                    }
                } else {

                    valor = bloco[j];
                    contador_tripla = 1;
                }
            }
        }

        /*
         * Sinaliza que terminou execução nesse bloco
         */
        pthread_mutex_lock(&lock_status);
        status[i % TAMANHO_DO_BUFFER]--;
        if (status[i % TAMANHO_DO_BUFFER] == 0) {

            pthread_cond_signal(
                    &cond_renova); //emite signal para desbloquear gerenciamento do buffer que ocorre na main :)
        }
        pthread_mutex_unlock(&lock_status);
        if (i != quantidade_de_blocos_totais - 1 && i != 0 && i % TAMANHO_DO_BUFFER == 0) {
            //se não for a última iteração no buffer e se for o último bloco do buffer, chegou na barreira
            barreira();
        }
    }
    pthread_exit((void *) qntd3); //retorna a quantidade de triplas encontradas no arquivo
}

void * conta_sequencias_de_0_a_5() {


    int *qntd0a5 = 0; //armazena a quantidade de ocorrências de sequências <012345>
    int valor; //variável auxiliar que armazena o valor anterior lido
    int em_sequencia = 0; //variável auxiliar que indica se uma sequência candidate está sendo lida
    int contador_aux; //outra variável auxiliar, que é incrementada conforme a sequência progride (ajuda a identificar)

    for(int i = 0; i < quantidade_de_blocos_totais; i++) { //laço que se repetirá na quantidade de blocos que serão lidos

        int *bloco; //ponteiro para inteiro que apontará para o bloco da iteração vigente
        long long int tamanho_laco;
        if (i == quantidade_de_blocos_totais - 1) {

            tamanho_laco = quantidade_de_inteiros_no_ultimo_bloco;
        } else {

            tamanho_laco = TAMANHO_DO_BLOCO;
        }
        bloco = buffer[i % TAMANHO_DO_BUFFER]; //busca o bloco no buffer

        for (int j = 0; j < tamanho_laco; j++) { //percorre o bloco e executa o algoritmo

            if (!em_sequencia) {

                if (bloco[i] == 0) {

                    valor = bloco[i];
                    em_sequencia = 1;
                }
                contador_aux = 0;
            } else {

                if (bloco[i] == valor + 1) {

                    contador_aux++;
                    if (contador_aux == 5) {

                        *qntd0a5++;
                        em_sequencia = 0;
                    }
                } else {

                    em_sequencia = 0;
                }
            }
        }

        /*
         * Sinaliza que terminou execução nesse bloco
         */
        pthread_mutex_lock(&lock_status);
        status[i % TAMANHO_DO_BUFFER]--;
        if (status[i % TAMANHO_DO_BUFFER] == 0) {

            pthread_cond_signal(
                    &cond_renova); //emite signal para desbloquear gerenciamento do buffer que ocorre na main :)
        }
        pthread_mutex_unlock(&lock_status);
        if (i != quantidade_de_blocos_totais - 1 && i != 0 && i % TAMANHO_DO_BUFFER == 0) {
            //se não for a última iteração no buffer e se for o último bloco do buffer, chegou na barreira
            barreira();
        }
    }
    pthread_exit((void *) qntd0a5); //retorna a quantidade de sequências <012345> encontradas no arquivo
}

void barreira() {

    pthread_mutex_lock(&lock_barreira);
    contador--;
    if(contador > 0) {

        pthread_cond_wait(&cond_bar, &lock_barreira);
    }
    else {

        contador = 4;
        pthread_cond_broadcast(&cond_bar);
    }
    pthread_mutex_unlock(&lock_barreira);
}