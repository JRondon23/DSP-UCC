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

#define TS 100000
#define PI 3.14159265358979
#define PTS 128
#define F1 100.0
#define F2 600.0
#define AMP1 10.0
#define AMP2 10.0
#define SAMPLING_FREQ 8000.0

short buffercount = 0;
short bufferfull=0;
float A[PTS], B[PTS], C[PTS];
float coeffs[PTS];
float *input_ptr, *output_ptr, *process_ptr, *temp_ptr;
float result[PTS];

FILE *out_f;

void tickConfig(unsigned int sampling_period);
void tickHandler(int sig, siginfo_t *si, void *uc);
void conv(float *x, float *y, float *z, int n);


int main()
{
    int i;

    out_f = fopen("archivo.txt", "w");

    for (i=0 ; i<PTS/2 ; i++) coeffs[i] = 0.0;
    for (i=0 ; i<N ; i++) coeffs[i] = h[i];

    input_ptr = A;
    output_ptr = B;
    process_ptr = C;

    tickConfig(TS);

    while(1)
    {
        while (bufferfull == 0);
        //wait for buffer full
        bufferfull = 0;
        temp_ptr = process_ptr;
        process_ptr = input_ptr;
        input_ptr = output_ptr;
        output_ptr = temp_ptr;

        conv(process_ptr, coeffs, result, PTS/2);

        for (i=0 ; i<PTS ; i++)
            *(process_ptr+i)=*(result+i);

        for (i=0 ; i<PTS/2 ; i++)
        {
            *(process_ptr + i) += *(output_ptr + i + PTS/2);
        }
    }

	fclose(out_f);

    return 0;
}


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


void tickHandler(int sig, siginfo_t *si, void *uc)
{
	static unsigned int n=0;
	float output_left_sample;
	float input;

    output_left_sample = (float)(*(output_ptr + buffercount));
    printf("%f     ", output_left_sample);

    input = (float)(AMP1*sin(2*PI*F1*n/SAMPLING_FREQ)) + (float)(AMP2*sin(2*PI*F2*n/SAMPLING_FREQ));

    *(input_ptr + buffercount++) = input;

    printf("%f\r\n", input);

    if (buffercount >= PTS/2)
    {
        bufferfull = 1;
        buffercount = 0;
        printf("nuevo bloque\r\n");
    }

    fprintf(out_f,"\n %d % 10.10f % 10.10f", buffercount, input, output_left_sample);

    n++;

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
