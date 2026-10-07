/* 
*
*   Lo que tengo que hacer es leer los coeficientes donde ingresan los filtro almacenarlos en algún 
*   arreglo, luego leo la señal y la almaceno en otro arreglo y luego realizo la convolución entre los dos
*
*/




#include <stdio.h>
#include <stdlib.h>
#include "filter.h"
#include "signal.h"
#include <math.h>
 

#define TAM_CONV LINE_ELEMENTS+N_FIR-1 //defino el tamaño de la convolucion

#define FS 8000






void generar_Conv(float coeficientes[N_FIR], short senal[LINE_ELEMENTS], float conv[TAM_CONV]); //Defino la función Convolución
void guardar_coeficientes(const char *nombre_archivo, const float *entrada, int tam);


int main() {

    float senal_filtrado[TAM_CONV] = {0};
    generar_Conv(h,signal,senal_filtrado); //aplico la convolución
    guardar_coeficientes("fir_out.txt",senal_filtrado,TAM_CONV);




    return 0;
}



void guardar_coeficientes(const char *nombre_archivo, const float *entrada, int tam)
{
  FILE *fir_out_f;
  fir_out_f = fopen(nombre_archivo, "w");
  if (fir_out_f == NULL)
  {
    fprintf(stderr, "Error al abrir el archivo %s para escritura.\n", nombre_archivo);
    return;
  }

  for (int n = 0; n < tam; n++)
  {
    fprintf(fir_out_f, "\n %f %15.15f", (float)n/FS, entrada[n]);
  }

  fclose(fir_out_f);
}


void generar_Conv(float coeficientes[N_FIR], short senal[LINE_ELEMENTS], float conv[TAM_CONV])
{
  for (int i = 0; i < TAM_CONV; i++)
  {
    conv[i] = 0.0f;
  }

  for (int n = 0; n < TAM_CONV; n++)
  {
    for (int k = 0; k < N_FIR; k++)
    {
      if ((n - k) >= 0 && (n - k) < LINE_ELEMENTS)
      {
        conv[n] += coeficientes[k] * senal[n - k];
      }
    }
  }
}