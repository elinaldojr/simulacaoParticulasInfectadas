#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define LARGURA 1024
#define ALTURA 1024
#define N 2048
#define CICLOS 20
#define TX_CONTAGIO 30 //30%

struct Vetor{
    float x, y;
};

struct Particula{
    struct Vetor posicao, velocidade, aceleracao;
    float raio, campo_visual, velocidade_max, forca_max;
    int infectada;
};

void inicializaParticulas(struct Particula particulas[]);
void imprimeParticulas(struct Particula particulas[]);
void inicializaInfeccao(struct Particula particulas[]);
void atualizaVetoresParticulas(struct Particula particulas[], struct Vetor separacao[], struct Vetor alinhamento[], struct Vetor coesao[]);
void atualizaPosicaoParticulas(struct Particula particulas[], struct Vetor separacao[], struct Vetor alinhamento[], struct Vetor coesao[]);
struct Vetor regraSeparacao(int indice, struct Particula particulas[]);
struct Vetor regraAlinhamento(int indice, struct Particula particulas[]);
struct Vetor regraCoesao(int indice, struct Particula particulas[]);
void regraContagio(int indice, struct Particula particulas[]);
int contaParticulasInfectadas(struct Particula particulas[]);

struct Vetor soma(struct Vetor A, struct Vetor B);
struct Vetor sub(struct Vetor A, struct Vetor B);
struct Vetor mult(struct Vetor A, float n);
struct Vetor divisao(struct Vetor A, float n);
float dist(struct Vetor A, struct Vetor B);
float magnitude(struct Vetor A);
struct Vetor defineMagnitude(struct Vetor A, float mag);
struct Vetor normaliza(struct Vetor A);
struct Vetor limita(struct Vetor A, float max);

int main() {
    struct Particula particulas[N];
    struct Vetor separacao[N], alinhamento[N], coesao[N];
    float largura_espaco, altura_espaco;
    long double vet[N];
    int i;
    double wtime;

    printf("Simulacao iniciada (PARALELO):\n\n");

    clock_t begin = clock();

    inicializaParticulas(particulas);
    inicializaInfeccao(particulas);

    for(i=0; i<CICLOS; i++){
        //printf("#Ciclo: %d\n", i);
        //printf("Total infectados: %d\n\n", infectados);
        //imprimeParticulas(particulas);
        atualizaVetoresParticulas(particulas, separacao, alinhamento, coesao);
        atualizaPosicaoParticulas(particulas, separacao, alinhamento, coesao);
    }

    clock_t end = clock();
    wtime = (double)(end - begin) / CLOCKS_PER_SEC;

    printf("#Ciclos: %d\n", i);
    printf("Total infectados: %d\n\n", contaParticulasInfectadas(particulas));
    printf("Tempo = %f s\n", wtime );

    return 0;
}

void inicializaParticulas(struct Particula particulas[]) {
    int i;
    FILE *arquivo;

    arquivo = fopen("particulas.txt", "r");

    for(i=0; i<N; i++){
       fscanf(arquivo, "%f %f %f %f %f %f %f %f %f %f %d", &particulas[i].posicao.x, &particulas[i].posicao.y
               , &particulas[i].velocidade.x, &particulas[i].velocidade.y
               , &particulas[i].aceleracao.x, &particulas[i].aceleracao.y
               , &particulas[i].raio, &particulas[i].campo_visual, &particulas[i].velocidade_max, &particulas[i].forca_max
               , &particulas[i].infectada);
    }
    fclose(arquivo);
}

void imprimeParticulas(struct Particula particulas[]) {
    int i;

    for(i=0; i<N; i++){
        printf("%d\nposicao: (%.2f, %.2f)\n", i, particulas[i].posicao.x, particulas[i].posicao.y);
        printf("velocidade: (%.2f, %.2f)\n", particulas[i].velocidade.x, particulas[i].velocidade.y);
        printf("aceleracao: (%.2f, %.2f)\n\n", particulas[i].aceleracao.x, particulas[i].aceleracao.y);
    }
}

void inicializaInfeccao(struct Particula particulas[]){
    int indice = rand()%N;

    particulas[indice].infectada = 1;
}

void atualizaVetoresParticulas(struct Particula particulas[], struct Vetor separacao[], struct Vetor alinhamento[], struct Vetor coesao[]){
    int i;

    #pragma omp parallel num_threads(16)
    {
        #pragma omp for schedule(dynamic, 1) private(i)
        for(i=0; i<N; i++){
            separacao[i] = regraSeparacao(i, particulas);
            alinhamento[i] = regraAlinhamento(i, particulas);
            coesao[i] = regraCoesao(i, particulas);

            if(particulas[i].infectada == 1){
                regraContagio(i, particulas);
            }
        }
    } //#pragma omp parallel
    #pragma omp barrier
}

void atualizaPosicaoParticulas(struct Particula particulas[], struct Vetor separacao[], struct Vetor alinhamento[], struct Vetor coesao[]){
    int i;

    #pragma omp parallel num_threads(16)
    {
        #pragma omp for schedule(dynamic, 1) private(i)
        for(i=0; i<N; i++){
            particulas[i].aceleracao = soma(particulas[i].aceleracao, separacao[i]);
            particulas[i].aceleracao = soma(particulas[i].aceleracao, alinhamento[i]);
            particulas[i].aceleracao = soma(particulas[i].aceleracao, coesao[i]);
            particulas[i].aceleracao = defineMagnitude(particulas[i].aceleracao, particulas[i].forca_max);

            particulas[i].velocidade = soma(particulas[i].velocidade, particulas[i].aceleracao);
            particulas[i].velocidade = limita(particulas[i].velocidade, particulas[i].velocidade_max);


            particulas[i].posicao = soma(particulas[i].posicao, particulas[i].velocidade);
            if(particulas[i].posicao.x > LARGURA){
                particulas[i].posicao.x = 0;
            }
            else if(particulas[i].posicao.x < 0){
                particulas[i].posicao.x = LARGURA;
            }

            if(particulas[i].posicao.y > ALTURA){
                particulas[i].posicao.y = 0;
            }
            else if(particulas[i].posicao.y < 0){
                particulas[i].posicao.y = ALTURA;
            }
        }
    } //#pragma omp parallel
    #pragma omp barrier
}

struct Vetor regraSeparacao(int indice, struct Particula particulas[]){
    struct Vetor velocidade_media, direcao, diferenca;
    float distancia, separacao_desejada;
    int i, cont = 0;

    velocidade_media.x = direcao.x = 0;
    velocidade_media.y = direcao.y = 0;

    separacao_desejada = 2*particulas[indice].raio;

    for(i=0; i<N; i++){
        distancia = dist(particulas[indice].posicao, particulas[i].posicao);

        if ((distancia>0) && (distancia<particulas[indice].campo_visual) && (distancia<separacao_desejada)){
            diferenca = sub(particulas[indice].posicao, particulas[i].posicao);
            diferenca = normaliza(diferenca);
            diferenca = divisao(diferenca, distancia);

            velocidade_media = soma(velocidade_media, diferenca);
            cont++;
        }
    }

    if(cont>0){
        velocidade_media = divisao(velocidade_media, cont);
        velocidade_media = defineMagnitude(velocidade_media, particulas[indice].velocidade_max);

        direcao = sub(velocidade_media, particulas[indice].velocidade);
        direcao = limita(direcao, particulas[indice].forca_max);
    }

    return direcao;
}

struct Vetor regraAlinhamento(int indice, struct Particula particulas[]){
    struct Vetor velocidade_media, direcao;
    float distancia;
    int i, cont=0;

    velocidade_media.x = direcao.x = 0;
    velocidade_media.y = direcao.y = 0;

    for(i=0; i<N; i++){
        distancia = dist(particulas[indice].posicao, particulas[i].posicao);

        if ((distancia>0) && (distancia<particulas[indice].campo_visual)){
            velocidade_media = soma(velocidade_media, particulas[i].velocidade);
            cont++;
        }
    }

    if(cont>0){
        velocidade_media = divisao(velocidade_media, cont);
        velocidade_media = defineMagnitude(velocidade_media, particulas[indice].velocidade_max);

        direcao = sub(velocidade_media, particulas[indice].velocidade);
        direcao = limita(direcao, particulas[indice].forca_max);
    }

    return direcao;
}

struct Vetor regraCoesao(int indice, struct Particula particulas[]){
    struct Vetor posicao_media, direcao;
    float distancia;
    int i, cont=0;

    posicao_media.x = direcao.x = 0;
    posicao_media.y = direcao.y = 0;

    for(i=0; i<N; i++){
        distancia = dist(particulas[indice].posicao, particulas[i].posicao);

        if ((distancia>0) && (distancia<particulas[indice].campo_visual)){
            posicao_media = soma(posicao_media, particulas[i].posicao);
            cont++;
        }
    }

    if(cont>0){
        posicao_media = divisao(posicao_media, cont);
        posicao_media = defineMagnitude(posicao_media, particulas[indice].velocidade_max);

        direcao = sub(posicao_media, particulas[indice].velocidade);
        direcao = limita(direcao, particulas[indice].forca_max);
    }

    return direcao;
}

void regraContagio(int indice, struct Particula particulas[]){
    float distancia, area_de_contagio;
    int i, chance_contagio;

    area_de_contagio = 20*particulas[indice].raio;

    for(i=0; i<N; i++){
        distancia = dist(particulas[indice].posicao, particulas[i].posicao);

        if ((distancia>0) && (distancia<area_de_contagio)){
            chance_contagio = rand()%100;
            if (chance_contagio < TX_CONTAGIO){
                particulas[i].infectada = 1;
            }
        }
    }

}

int contaParticulasInfectadas(struct Particula particulas[]){
    int i, infectados = 0;
    for(i=0; i<N;i++){
        if(particulas[i].infectada == 1)
            infectados++;
    }
    return infectados;
}

struct Vetor soma(struct Vetor A, struct Vetor B){
    struct Vetor C;

    C.x = A.x + B.x;
    C.y = A.y + B.y;

    return C;
}

struct Vetor sub(struct Vetor A, struct Vetor B){
    B.x = -B.x;
    B.y = -B.y;

    return soma(A, B);
}

struct Vetor mult(struct Vetor A, float n){
    A.x = A.x *n;
    A.y = A.y *n;

    return A;
}

struct Vetor divisao(struct Vetor A, float n){
    return mult(A, 1/n);
}

float dist(struct Vetor A, struct Vetor B){
    return sqrt(pow(B.x - A.x, 2) + pow(B.y - A.y, 2));
}

float magnitude(struct Vetor A){
    return sqrt(A.x*A.x + A.y*A.y);
}

struct Vetor defineMagnitude(struct Vetor A, float mag){
    A = normaliza(A);

    return mult(A, mag);
}

struct Vetor normaliza(struct Vetor A){
    float mag = magnitude(A);
    if(mag != 0)
        return divisao(A, mag);

    return A;
}

struct Vetor limita(struct Vetor A, float max){
    if(magnitude(A) > max)
        return defineMagnitude(A, max);

    return A;
}
