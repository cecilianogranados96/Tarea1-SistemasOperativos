#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

struct parametros{
    int *inicio;
    size_t tamano;
    int profundidad;
};

void *ordenar_hilos(void *pv);

//FUNCION DE MERGE BASICA
void merge(int *inicio, int *mid, int *fin){
    int *res = malloc((fin - inicio)*sizeof(*res));
    int *lhs = inicio, *rhs = mid, *dst = res;
    while (lhs != mid && rhs != fin)
        *dst++ = (*lhs < *rhs) ? *lhs++ : *rhs++;
    while (lhs != mid)
        *dst++ = *lhs++;
    memcpy(inicio, res, (rhs - inicio) * sizeof *res);
    free(res);
}

//METODO DE MULTIHILO TIENE UNA PROFUNDIDAD DADA
void ordenar_generando_hilos(int *inicio, size_t tamano, int profundidad){
    if (tamano < 2)
        return;
    if (profundidad <= 0 || tamano < 4){
        ordenar_generando_hilos(inicio, tamano/2, 0);
        ordenar_generando_hilos(inicio+tamano/2, tamano-tamano/2, 0);
    }else{
        struct parametros params = { inicio, tamano/2, profundidad/2 };
        pthread_t thrd;
        printf("Inicio del hilo...\n");
        pthread_create(&thrd, NULL, ordenar_hilos, &params);  // CREA EL HILO MANDANDO PARAMETROS LA FUNCION
        ordenar_generando_hilos(inicio+tamano/2, tamano-tamano/2, profundidad/2); // Llamada recursiva en el orden del hilo
        pthread_join(thrd, NULL);  // Cuando termina se une al hilo principal
        printf("Fin del subhilo.\n");
    }
    merge(inicio, inicio+tamano/2, inicio+tamano);
}

//LLAMA ALA FUNCION REAL CON LOS PARAMETROS ES LA UNICA FORMA DE LLAMARLO
void *ordenar_hilos(void *pv)
{
    struct parametros *params = pv;
    ordenar_generando_hilos(params->inicio, params->tamano, params->profundidad);
    return pv;
}

int main(){
    int cantidad = 2048;
    
    int *datos = malloc(cantidad * sizeof(*datos));
    srand((unsigned)time(0));
    
    for (int i=0; i<cantidad; i++){
        datos[i] = rand() % 1024;
        printf("%4d \t", datos[i]);
        if ((i+1)%9 == 0)
            printf("\n");
    }
    printf("\n");

    ordenar_generando_hilos(datos, cantidad, 5); // 5 es un buen numero para la cantidad de hilos
        
    for (int i=0; i<cantidad; i++){
        printf("%4d \t", datos[i]);
        if ((i+1)%9 == 0)
            printf("\n");
    }
    printf("\n");

    return 0;
}