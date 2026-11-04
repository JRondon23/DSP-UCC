/* Simulador de escritorio: usa exactamente el nucleo y los coeficientes STM32.
 * Uso: iir_sim.exe 0|1 entrada.txt salida.txt (0: pasabanda; 1: rechazo). */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "iir_sos.h"
#include "iir_coeffs.h"

int main(int argc, char **argv)
{
    if (argc != 4 || (argv[1][0] != '0' && argv[1][0] != '1'))
        return 1;
    int rechazo = argv[1][0] == '1';
    FILE *in = fopen(argv[2], "r");
    FILE *out = fopen(argv[3], "w");
    if (!in || !out) return 2;
    IIR_State bp[IIR_PASABANDA_SECTIONS] = {0};
    IIR_State bs[IIR_RECHAZABANDA_SECTIONS] = {0};
    float x;
    while (fscanf(in, "%f", &x) == 1) {
        float y = rechazo
            ? iir_process(x, iir_rechazabanda, bs, IIR_RECHAZABANDA_SECTIONS)
            : iir_process(x, iir_pasabanda, bp, IIR_PASABANDA_SECTIONS);
        if (!isfinite(y)) return 3;
        fprintf(out, "%.9g\n", y);
    }
    if (!feof(in)) return 4;
    fclose(in);
    return fclose(out) != 0;
}
