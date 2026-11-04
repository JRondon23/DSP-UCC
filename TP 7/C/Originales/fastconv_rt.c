#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include "filter.h"

#define TS 100000 // periodo de muestreo en nanosegundos

FILE *out_f;
FILE *out_f2;

struct cmpx //complex data structure used by FFT
{
	float real;
	float imag;
};

typedef struct cmpx COMPLEX;

#define TESTFREQ 200.0
#define TESTFREQ2 600.0
#define SAMPLING_FREQ 8000.0

#define PI 3.14159265358979
#define PTS 128

void tickConfig(unsigned int sampling_period);
void tickHandler(int sig, siginfo_t *si, void *uc);
void fft(COMPLEX *Y, int M, COMPLEX *w);

short buffercount = 0; //indice de los elementos de los bloques
short bufferfull=0;
COMPLEX A[PTS], B[PTS], C[PTS];
COMPLEX twiddle[PTS]; // factores de giro
COMPLEX twiddle_conj[PTS]; // factores de giro conjugados
COMPLEX coeffs[PTS]; // coeficientes de frecuencia rellenados con ceros
COMPLEX *input_ptr, *output_ptr, *process_ptr, *temp_ptr;
float a, b; // variables usadas en la multiplicacion compleja


int main()
{
	int n, i;

	out_f = fopen("archivo.txt", "w");
	out_f2 = fopen("archivo2.txt", "w");

	for (n=0; n<PTS; n++) // genera los factores de giro
	{
		twiddle[n].real = cos(PI*n/PTS);
		twiddle[n].imag = -sin(PI*n/PTS);
	}
	
	for (n=0; n<PTS; n++) // genera los factores de giro conjugados
	{
		twiddle_conj[n].real = cos(PI*n/PTS);
		twiddle_conj[n].imag = sin(PI*n/PTS);
	}
	
	for (n=0; n<PTS; n++) // inicializa con ceros los coeficientes del filtro en el dominio de la frecuencia
	{
		coeffs[n].real = 0.0;
		coeffs[n].imag = 0.0;
	}
	for (n=0; n<N; n++)
	{
		coeffs[n].real = h[n];
	}

	fft(coeffs, PTS, twiddle); // transforma los coeficietes del filtro al dominio de la frecuencia

	input_ptr = A; // iniciliza los punteros a los bloques
	process_ptr = B;
	output_ptr = C;

	tickConfig(TS);

	while(1) // lazo de procesamiento de bloque
	{
		while (bufferfull == 0); // espera a que se llene el buffer con PTS/2 muestras
		bufferfull = 0;
		temp_ptr = process_ptr;
		process_ptr = input_ptr;
		input_ptr = output_ptr;
		output_ptr = temp_ptr;
		
		for (i=0; i< PTS; i++) (process_ptr + i)->imag = 0.0; // pone a cero el buffer completo de la parte imaginaria, la entrada es completamente real
		for (i=PTS/2; i< PTS; i++) (process_ptr + i)->real = 0.0; // rellena con ceros la parte alta del buffer que contiene las muestras de entrada
									  // los bloques son de tamanio PTS/2, por eso se pone a cero la parte alta del buffer 

		fft(process_ptr, PTS, twiddle); // transforma las muestras de entrada al dominio de la frecuencisa usando la FFT

		for (i=0; i<PTS; i++)   // se realiza el filtrado en el dominio de la frecuencia, es decir, se realiza una multiplicacion compleja			
		{			// que equivale a la convolución en el dominio del tiempo
			a = (process_ptr + i)->real;
			b = (process_ptr + i)->imag;
			(process_ptr + i)->real = coeffs[i].real*a - coeffs[i].imag*b;
			(process_ptr + i)->imag = (coeffs[i].real*b + coeffs[i].imag*a);
		}

		fft(process_ptr, PTS, twiddle_conj); // realiza la FFT inversa
		for (i=0; i<PTS; i++)
		{
			(process_ptr + i)->real /= PTS;
			(process_ptr + i)->imag /= PTS;
		}

		for (i=0; i<PTS/2; i++) // superponer-sumar solo la parte real
		{
			(process_ptr + i)->real += (output_ptr + i + PTS/2)->real;
		}
	}

	fclose(out_f);
	fclose(out_f2);

	return 0;
}


void tickHandler(int sig, siginfo_t *si, void *uc)
{
	static unsigned int n=0;
	float output_left_sample;
	float input;

	output_left_sample=(output_ptr + buffercount)->real;
	printf("%f     ", output_left_sample);

	input = (float)(sin(2*PI*TESTFREQ*n/SAMPLING_FREQ)) + (float)(50*sin(2*PI*TESTFREQ2*n/SAMPLING_FREQ));
	(input_ptr + buffercount)->real = input;
	(input_ptr + buffercount++)->imag = 0.0;

	printf("%f\r\n", input);
	
	if (buffercount >= PTS/2)
	{
		buffercount = 0;
		bufferfull = 1;
		printf("nuevo bloque\r\n");
	}

	n++;

	fprintf(out_f,"\n %d % 10.10f % 10.10f", buffercount, input, output_left_sample);

	return;
}


void tickConfig(unsigned int sampling_period){
    
	timer_t timerid;
	struct sigevent sev;
	struct itimerspec its;
	struct sigaction sa;

	sa.sa_flags = 0;
	sa.sa_sigaction = tickHandler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGALRM, &sa, NULL) == -1)
		printf("error sigaction");

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGALRM;
	sev.sigev_value.sival_ptr = &timerid;
	if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
		printf("error timer_create");

	its.it_value.tv_sec = 1;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = sampling_period;

	if (timer_settime(timerid, 0, &its, NULL) == -1)
		printf("error timer_settime");
    
	return; 
}


void fft(COMPLEX *Y, int M, COMPLEX *w)
{
	COMPLEX temp1,temp2; //temporary storage variables
	int i,j,k; //loop counter variables
	int upper_leg, lower_leg; //index of upper/lower butterfly leg
	int leg_diff; //difference between upper/lower leg
	int num_stages=0; //number of FFT stages, or iterations
	int index, step; //index and step between twiddle factor
	
	i=1; //log(2) of # of points = # of stages
	
	do
	{
		num_stages+=1;
		i=i*2;
	} while (i!=M);
	
	leg_diff=M/2; //difference between upper & lower legs
	step=2; //step between values in twiddle.h
	
	for (i=0;i<num_stages;i++)
	{
		index=0;
		for (j=0;j<leg_diff;j++)
		{
			for (upper_leg=j;upper_leg<M;upper_leg+=(2*leg_diff))
			{
				lower_leg=upper_leg+leg_diff;
				temp1.real=(Y[upper_leg]).real + (Y[lower_leg]).real;
				temp1.imag=(Y[upper_leg]).imag + (Y[lower_leg]).imag;
				temp2.real=(Y[upper_leg]).real - (Y[lower_leg]).real;
				temp2.imag=(Y[upper_leg]).imag - (Y[lower_leg]).imag;
				(Y[lower_leg]).real=temp2.real*(w[index]).real
				-temp2.imag*(w[index]).imag;
				(Y[lower_leg]).imag=temp2.real*(w[index]).imag
				+temp2.imag*(w[index]).real;
				(Y[upper_leg]).real=temp1.real;
				(Y[upper_leg]).imag=temp1.imag;
			}
			index+=step;
		}
		leg_diff=leg_diff/2;
		step*=2;
	}
	
	j=0;
	
	for (i=1;i<(M-1);i++) //bit reversal for resequencing data
	{
		k=M/2;
		while (k<=j)
		{
			j=j-k;
			k=k/2;
		}
		j=j+k;
		if (i<j)
		{
			temp1.real=(Y[j]).real;
			temp1.imag=(Y[j]).imag;
			(Y[j]).real=(Y[i]).real;
			(Y[j]).imag=(Y[i]).imag;
			(Y[i]).real=temp1.real;
			(Y[i]).imag=temp1.imag;
		}
	}
	
	for(k=0;k<N;k++){
		fprintf(out_f2,"\n %d % 10.10f % 10.10f", k, Y[k].real, Y[k].imag);
	}
	
	return;
}
