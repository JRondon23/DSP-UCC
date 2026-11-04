
#include <stdio.h>
#include <math.h>
#include "filter.h"

#define PI 3.14159265358979
#define PTS 128

#define NS 20
#define FS 8000
#define F1 100
#define F2 1000

float coeffs[PTS/2]; // zero-padded filter coeffs
float A[PTS], B[PTS], C[PTS]; // buffers
float *input_ptr, *output_ptr, *temp_ptr, *process_ptr;
float result[PTS]; // temporary storage

short i;
int wt = 0;

FILE *out_f1;
FILE *out_f2;

void conv(float *x, float *y, float *z, int n)
{
	int i, n_lo, n_hi;
	float *xp, *yp;
	for(i=0;i<(2*n-1);i++)
	{
		*z=0.0;
		n_lo=i-(n)+1;
		if(n_lo<0)n_lo=0;
		n_hi=i;
		if(n_hi>(n-1))n_hi=(n-1);
		for(xp=x+n_lo,yp=y+i-n_lo;xp<=x+n_hi;xp++,yp--)
		*z+=*xp * *yp;
		z++;
	}
	*z=0.0;
}


int main()
{
	int n;
	input_ptr = A; // initialise pointers
	output_ptr = B;
	process_ptr = C;
	int ns = 0; // numero de muestra de la señal de entrada

	out_f1 = fopen("archivo_out.txt", "w");
	out_f2 = fopen("archivo_in.txt", "w");

	for (i=0 ; i<PTS/2 ; i++) coeffs[i] = 0.0; // zero pad filter

	for (i=0 ; i<N ; i++) coeffs[i] = h[i]; // in array coeffs

	for (i=0 ; i<PTS ; i++) // zero all buffer contents
	{
		*(output_ptr + i) = 0.0;
		*(process_ptr + i) = 0.0;
		*(input_ptr + i) = 0.0;
	}

	for (n=0; n < NS; n++) // loop
	{
		conv(process_ptr, coeffs, result, PTS/2); // convolve contents
		for (i=0 ; i<PTS ; i++) // of process buffer
		*(process_ptr+i)=*(result+i); // with filter coeffs

		for (i=0 ; i<PTS/2 ; i++) // read new input
		{
			*(input_ptr + i) = (float)(sin(2*PI*F1*ns/FS)) +  (float)(sin(2*PI*F2*ns/FS));
			ns++;

			fprintf(out_f2,"\n % 10f", *(input_ptr + i));
		}

		for (i=0 ; i<PTS/2 ; i++) // add overlapping output
		{ //sections in process buffer
			*(process_ptr + i) += *(output_ptr + i + PTS/2);

			fprintf(out_f1,"\n % 10f", *(process_ptr + i));
		}

		temp_ptr = process_ptr; // rotate input, output,
		process_ptr = input_ptr; // and process buffers
		input_ptr = output_ptr;
		output_ptr = temp_ptr;
	}

	fclose(out_f1);
	fclose(out_f2);

	return 0;
}
