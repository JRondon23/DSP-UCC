/* 
*
*   Lo que tengo que hacer es leer los coeficientes donde ingresan los filtro almacenarlos en algún 
*   arreglo, luego leo la señal y la almaceno en otro arreglo y luego realizo la convolución entre los dos
*
*/




#include <stdio.h>
#include <stdlib.h>
#include "filter_IIR.h"
#include "signal_IIR.h"
#include <math.h>
 

#define TAM_CONV LINE_ELEMENTS+N_iir-1 //defino el tamaño de la convolucion

#define FS 8000






void generar_Conv(float arreglo_alfa[N_iir], float arreglo_beta[N_iir], float conv[TAM_CONV], short signal[LINE_ELEMENTS]);
void guardar_coeficientes(const char *nombre_archivo, const float *entrada, int tam);


int main() {

    float senal_filtrado[TAM_CONV] = {0};
    generar_Conv(b,a,senal_filtrado,signal); //aplico la convolución
    guardar_coeficientes("iir_out.txt",senal_filtrado,TAM_CONV);




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


void generar_Conv(float arreglo_alfa[N_iir], float arreglo_beta[N_iir], float conv[TAM_CONV], short signal[LINE_ELEMENTS])
{
  for (int n = 0; n < TAM_CONV; n++)
  {
    conv[n] = 0.0f;
  }

  for (int k = 0; k < TAM_CONV; k++)
  {
    float sum_alfa = 0.0f, sum_beta = 0.0f;
    for (int n = 0; n < N_iir; n++)
    {
      if (k - n >= 0)
      {
        if (k - n < LINE_ELEMENTS)
        {
          sum_alfa += arreglo_alfa[n] * signal[k - n];
        }

        if (n != 0)
        {
          sum_beta += arreglo_beta[n] * conv[k - n];
        }
      }
    }
    conv[k] = sum_alfa - sum_beta;
  }
}