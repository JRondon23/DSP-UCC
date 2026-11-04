#ifndef IIR_SOS_H
#define IIR_SOS_H

/* Forma directa II transpuesta. Una fila: b0,b1,b2,a0=1,a1,a2.
 * Los signos de a1 y a2 son los de Octave, sin invertirlos al exportar. */
typedef struct {
    float z1;
    float z2;
} IIR_State;

static inline float iir_process(float x, const float sos[][6],
                                IIR_State state[], unsigned sections)
{
    for (unsigned s = 0; s < sections; ++s) {
        const float *c = sos[s];
        float y = c[0] * x + state[s].z1;
        state[s].z1 = c[1] * x - c[4] * y + state[s].z2;
        state[s].z2 = c[2] * x - c[5] * y;
        x = y;
    }
    return x;
}
#endif
