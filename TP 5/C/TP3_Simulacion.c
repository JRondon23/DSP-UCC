#include <stdio.h>
#include <stdlib.h>
#include "test_signal.h"
#include <math.h>

#ifndef ELEMENTS
#define ELEMENTS 2000
#endif

#define NS 2000
#define FS 8000

// Constante para el tamaño de la convolución (NS + 2*NS - 1 = 3999 muestras)
#define TAM_CONV ((2 * NS) - 1)

// --- PROTOTIPOS DE FUNCIONES CORREGIDOS ---
void Procesar_Senal(const float *entrada, float *salida, int tam);
void Guardar_Resultados(const char *nombre_archivo, const float *entrada, const float *salida, int tam);
void Aplicar_Delay(const float *entrada, float *salida, int tam, int delay_muestras);
void Generar_Seno(float Seno_C[NS]);
void Generar_Exp(float Exp[2 * NS]);
void Generar_Random(float Random[NS]);
void Generar_Conv(float Seno_C[NS], float Exp[2 * NS], float Conv[TAM_CONV]);

// Arreglos globales
float Seno_C[NS] = {0};
float Exp[2 * NS] = {0};
float Random[NS] = {0};
float Conv[TAM_CONV] = {0};

// --- MAIN ---
int main()
{
  // Búfer del tamaño adecuado para almacenar la convolución procesada (3999 muestras)
  float output_signal[TAM_CONV];

  // 1. Generamos las señales base
  Generar_Seno(Seno_C);
  Generar_Exp(Exp);

  // 2. Calculamos la convolución (Seno_C * Exp)
  Generar_Conv(Seno_C, Exp, Conv);

  // 3. Procesamos la señal convolucionada (aplicamos ganancia/offset)
  Procesar_Senal(Conv, output_signal, TAM_CONV);

  // 4. Guardamos en archivo1.txt: Columna 1 = Tiempo, Columna 2 = Entrada (Conv), Columna 3 = Salida (output_signal)
  Guardar_Resultados("archivo1.txt", Exp, output_signal, ELEMENTS);

  printf("Procesamiento completado. Convolución (%d muestras) guardada en 'archivo1.txt'.\n", TAM_CONV);

  return 0;
}

// --- IMPLEMENTACIÓN DE FUNCIONES ---

// Se cambió 'const short *entrada' a 'const float *entrada'
void Procesar_Senal(const float *entrada, float *salida, int tam)
{
  float gain = 1.0f;  // ganancia
  float level = 0.0f; // nivel de offset

  for (int n = 0; n < tam; n++)
  {
    salida[n] = gain * entrada[n] + level;
  }
}

// Se cambió 'const short *entrada' a 'const float *entrada'
void Guardar_Resultados(const char *nombre_archivo, const float *entrada, const float *salida, int tam)
{
  FILE *out_f = fopen(nombre_archivo, "w");

  if (out_f == NULL)
  {
    fprintf(stderr, "Error al abrir el archivo %s para escritura.\n", nombre_archivo);
    return;
  }

  // Cabecera inicial para Octave
  fprintf(out_f, "Tiempo Entrada Salida\n");

  for (int n = 0; n < tam; n++)
  {
    float tiempo = (float)n / FS;
    fprintf(out_f, "%10f %10f %10f\n", tiempo, entrada[n], salida[n]);
  }

  fclose(out_f);
}

void Aplicar_Delay(const float *entrada, float *salida, int tam, int delay_muestras)
{
  for (int n = 0; n < tam; n++)
  {
    if (n < delay_muestras)
    {
      salida[n] = 0.0f;
    }
    else
    {
      salida[n] = entrada[n - delay_muestras];
    }
  }
}

void Generar_Seno(float Seno_C[NS])
{
  float f = 100.0f;
  float A = 100.0f;
  float fs = FS;

  for (int n = 0; n < NS; n++)
  {
    Seno_C[n] = (A * sinf(2.0f * (float)M_PI * f * (float)n / fs));
  }
}

void Generar_Exp(float Exp[2 * NS])
{
  for (int n = 0; n < 2 * NS; n++)
  {
    if (n < NS)
    {
      Exp[n] = expf((float)n / 500.0f);
    }
    else
    {
      Exp[n] = 0.0f;
    }
  }
}

void Generar_Random(float Random[NS])
{
  float A = 10 * 1e-3f;
  for (int n = 0; n < NS; n++)
  {
    Random[n] = (A * (float)rand());
  }
}

void Generar_Conv(float Seno_C[NS], float Exp[2 * NS], float Conv[TAM_CONV])
{
  for (int i = 0; i < TAM_CONV; i++)
  {
    Conv[i] = 0.0f;
  }

  for (int n = 0; n < TAM_CONV; n++)
  {
    for (int k = 0; k < NS; k++)
    {
      if ((n - k) >= 0 && (n - k) < (2 * NS))
      {
        Conv[n] += Seno_C[k] * Exp[n - k];
      }
    }
  }
}