#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#import <string.h>

int main(int argc, char *argv[]) {
    //A entrada do programa deve a quantidade de casos de teste desejada e o tamanho de tais casos

    if(argc != 4){
        fprintf(stderr, "Por favor, insira na entrada: <casos_teste>, <tamanho_teste>, <nome_arquivo>");
        return 1;
    }

    int casos_teste = atoll(argv[1]);
    int tamanho_teste = atoll(argv[2]);
    char nome_arquivo[40];
    strcpy(nome_arquivo, argv[3]);
    srand(time(NULL)); // inicializacao da semente

    FILE *arq;
    arq = fopen(nome_arquivo, "w");

    if(arq == NULL){
        printf("Erro na criacao do arquivo");
    }

    else{
        //Caso o arquivo tenha sido aberto, escrevemos no arquivo binario

        for (long long int i = 0; i < casos_teste; i++){  //Aqui devem ser escritos os vetores para teste no arquivo binário,
            int vetor[tamanho_teste];                                    // cada um contendo "tamanho_teste" elementos
            for(long long int j = 0; j < tamanho_teste; j++){
                int r = rand() % 101; //Inteiro aleatório entre 0 e 100
                vetor[j] = r;
            }
            fwrite(vetor, sizeof(int), tamanho_teste, arq);
            char separacao = "\n";
            fwrite(separacao, sizeof(char), 1, arq);
        }
    }

    return 0;
}
