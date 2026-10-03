#include <stdio.h>

#include "filter.h"
#include "signal.h"

#define NS 4000
#define FS 8000
#define PI 3.1415926


int main()
{
	long n, i;
	float fir_out;
	float fir_input[N_FIR+1] = {0.0};

	FILE *fir_out_f;
	fir_out_f = fopen("fir_out.txt", "w");


	for (n = 0; n < NS; n++)
	{
		fir_input[0] = signal[n];
		
		fir_out = 0;
		for(i = N_FIR-1; i >= 0; i--)
		{
    		    fir_out += (h[i] * fir_input[i]);
    		    fir_input[i+1] = fir_input[i];
		}

		fprintf(fir_out_f, "\n %f %15.15f", (float)n/FS, fir_out);
	}

	fclose(fir_out_f);

	return 0;
}
