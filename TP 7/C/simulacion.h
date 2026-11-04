/* Simulacion finita de adquisicion por bloques; no usa temporizadores de SO. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "filter.h"
#define BLOCK 128
#define FFT_SIZE 256
#define PI 3.14159265358979323846
typedef struct { double r, i; } Complex;

/* FFT radix-2, orden natural; inversa normalizada por FFT_SIZE. */
static void fft(Complex *a, int inverse) {
    for (unsigned i=1,j=0;i<FFT_SIZE;i++) {
        unsigned bit=FFT_SIZE/2;
        for (;j&bit;bit>>=1) j^=bit;
        j^=bit;
        if (i<j) { Complex t=a[i]; a[i]=a[j]; a[j]=t; }
    }
    for (unsigned len=2;len<=FFT_SIZE;len<<=1) {
        double angle=(inverse?2:-2)*PI/len;
        for (unsigned start=0;start<FFT_SIZE;start+=len) {
            Complex w={1,0};
            for (unsigned j=0;j<len/2;j++) {
                Complex u=a[start+j], b=a[start+j+len/2];
                Complex v={b.r*w.r-b.i*w.i,b.r*w.i+b.i*w.r};
                a[start+j]=(Complex){u.r+v.r,u.i+v.i};
                a[start+j+len/2]=(Complex){u.r-v.r,u.i-v.i};
                w=(Complex){w.r*cos(angle)-w.i*sin(angle),w.r*sin(angle)+w.i*cos(angle)};
            }
        }
    }
    if (inverse) for (int i=0;i<FFT_SIZE;i++) { a[i].r/=FFT_SIZE; a[i].i/=FFT_SIZE; }
}

int main(int argc, char **argv) {
    if (argc<3 || argc>4) { fprintf(stderr,"Uso: programa ruido|senos|impulso salida.csv [muestras]\n"); return 1; }
    int mode=!strcmp(argv[1],"ruido")?0:!strcmp(argv[1],"senos")?1:!strcmp(argv[1],"impulso")?2:-1;
    char *end=NULL;
    long count=argc==4?strtol(argv[3],&end,10):8192;
    if (mode<0 || count<1 || count>1000000 || (argc==4 && *end)) return 1;
    FILE *out=fopen(argv[2],"w"); if (!out) { perror(argv[2]); return 1; }
    double tail[N-1]={0}; uint32_t seed=7;
    Complex H[FFT_SIZE]={{0}};
    if (USE_FFT) { for (int k=0;k<N;k++) H[k].r=h[k]; fft(H,0); }
    fprintf(out,"n,t_s,x,y\n");
    long total=count+N-1;
    for (long start=0; start<total; start+=BLOCK) {
        double x[BLOCK]={0}, y[FFT_SIZE]={0};
        for (int j=0;j<BLOCK;j++) {
            long n=start+j;
            if (n>=count) continue;
            if (mode==0) { seed=1664525u*seed+1013904223u; x[j]=2.0*(double)seed/4294967296.0-1; }
            if (mode==1) x[j]=sin(2*PI*100*n/8000)+sin(2*PI*1000*n/8000)+sin(2*PI*2000*n/8000);
            if (mode==2) x[j]=(n==0);
        }
        if (USE_FFT) {
            Complex a[FFT_SIZE]={{0}};
            for (int j=0;j<BLOCK;j++) a[j].r=x[j];
            fft(a,0);
            for (int j=0;j<FFT_SIZE;j++) a[j]=(Complex){a[j].r*H[j].r-a[j].i*H[j].i,a[j].r*H[j].i+a[j].i*H[j].r};
            fft(a,1); for (int j=0;j<FFT_SIZE;j++) y[j]=a[j].r;
        } else {
            for (int j=0;j<BLOCK;j++) for (int k=0;k<N;k++) y[j+k]+=x[j]*h[k];
        }
        for (int j=0;j<N-1;j++) y[j]+=tail[j];
        for (int j=0;j<N-1;j++) tail[j]=y[BLOCK+j];
        for (int j=0;j<BLOCK && start+j<total;j++)
            fprintf(out,"%ld,%.17g,%.17g,%.17g\n",start+j,(start+j)/8000.0,x[j],y[j]);
    }
    return fclose(out)==0?0:1;
}
