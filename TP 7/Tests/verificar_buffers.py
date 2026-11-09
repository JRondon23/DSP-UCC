"""Ejercita la logica real de main.c con HAL simulada, sin conectar la placa.

Extrae las zonas USER CODE para no mantener una copia del algoritmo en el test.
Requiere Python 3 y GCC para PC en PATH. No ejecuta PowerShell ni flashea firmware.
"""
from pathlib import Path
import re
import subprocess
import tempfile

base = Path(__file__).resolve().parents[1]
source = (base / 'MXCube/TP7/Core/Src/main.c').read_text(encoding='utf-8')


def section(name):
    return re.search(
        r'/\* USER CODE BEGIN ' + re.escape(name) + r' \*/(.*?)'
        r'/\* USER CODE END ' + re.escape(name) + r' \*/', source, re.S
    ).group(1)


stubs = r'''
#include <stdint.h>
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "filter.h"
typedef struct { void *Instance; } ADC_HandleTypeDef;
typedef struct { void *Instance; } DAC_HandleTypeDef;
typedef struct { void *Instance; } TIM_HandleTypeDef;
#define ADC1 ((void *)1)
#define TIM8 ((void *)2)
#define DAC_CHANNEL_2 2
#define DAC_ALIGN_12B_R 0
#define HAL_OK 0
ADC_HandleTypeDef hadc1={ADC1};
DAC_HandleTypeDef hdac1={0};
static uint32_t sample, primask, last_dac;
static jmp_buf trap;
static uint32_t __get_PRIMASK(void) { return primask; }
static void __disable_irq(void) { primask=1; }
static void __set_PRIMASK(uint32_t v) { primask=v; }
static void __DMB(void) { }
static uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *h) { (void)h; return sample; }
static int HAL_ADC_Start_IT(ADC_HandleTypeDef *h) { (void)h; return HAL_OK; }
static int HAL_DAC_SetValue(DAC_HandleTypeDef *h,int c,int a,uint32_t v)
{ (void)h; (void)c; (void)a; last_dac=v; return HAL_OK; }
void Error_Handler(void) { longjmp(trap,1); }
'''

tests = r'''
/* Referencia independiente: convolucion directa de toda la senal en double,
 * con los coeficientes originales de Octave, sin dividir en bloques. */
static double reference_input[4096];
static double reference_value(uint32_t n) {
  double y=0;
  for (uint32_t k=0;k<N;k++)
    if (n>=k) y+=(reference_input[n-k]-2048.0)*h[k];
  return y;
}
static void check_code(uint32_t got,uint32_t n) {
  double y=reference_value(n)+2048.0;
  if (y<0) y=0; else if (y>4095) y=4095;
  /* Como maximo una cuenta por redondeo cerca de un medio entero. */
  assert(fabs((double)got-floor(y+0.5))<=1.0);
}
static void check_fir(void) {
  /* Impulsos en ambas fronteras de bloque, continua, ruido y tres tonos.
   * Los ultimos dos bloques son entrada centrada nula para vaciar la cola. */
  for (uint32_t mode=0;mode<5;mode++) {
    memset(fir_tail,0,sizeof(fir_tail));
    uint32_t seed=7;
    for (uint32_t i=0;i<768;i++) {
      double x=0;
      if (i<512) {
        if (mode==0) x=(i==127?1000:0);
        if (mode==1) x=(i==128?1000:0);
        if (mode==2) x=800;
        if (mode==3) { seed=1664525u*seed+1013904223u; x=(double)(seed%3001)-1500; }
        if (mode==4) x=(int)(400*(sin(2*3.141592653589793*100*i/8000)+
                sin(2*3.141592653589793*1000*i/8000)+sin(2*3.141592653589793*2000*i/8000)));
      }
      reference_input[i]=x+2048;
    }
    for (uint32_t block=0;block<6;block++) {
      for (uint32_t i=0;i<128;i++) input_block[i]=(float)(reference_input[block*128+i]-2048);
      TP7_FilterInputBlock();
      for (uint32_t i=0;i<128;i++)
        assert(fabs((double)fir_result[i]-reference_value(block*128+i))<0.01);
    }
    for (uint32_t i=0;i<128;i++) assert(fir_tail[i]==0.0f);
  }
  memset(fir_tail,0,sizeof(fir_tail));
}
static void feed(uint32_t value) {
  sample=value; adc_pending=1;
  HAL_ADC_ConvCpltCallback(&hadc1);
  assert(adc_pending==0);
}
int main(void) {
  check_fir();
  for (uint32_t i=0;i<128;i++) {
    reference_input[i]=1000+i;
    reference_input[128+i]=3000+i;
    reference_input[256+i]=(i==0?0:i==1?2048:4095);
  }
  /* Ningun bloque se entrega antes de tener exactamente 128 muestras. */
  for (uint32_t i=0;i<127;i++) {
    feed(1000+i); TP7_PrepareInputBlock();
    assert(input_blocks_prepared==0);
  }
  feed(1127);
  assert(input_blocks_acquired==1);
  assert(input_state[0]==TP7_BUFFER_READY);
  assert(input_state[1]==TP7_BUFFER_FILLING);

  /* La adquisicion del segundo buffer no altera el primero. */
  feed(3000);
  TP7_PrepareInputBlock();
  for (uint32_t i=0;i<128;i++)
    assert(input_block[i]==(float)(1000+i)-2048.0f);
  assert(input_state[0]==TP7_BUFFER_FREE);
  assert(input_blocks_prepared==1);
  TP7_PrepareInputBlock();
  assert(input_blocks_prepared==1); /* No entregar dos veces. */
  for (uint32_t i=1;i<128;i++) feed(3000+i);
  TP7_PrepareInputBlock();
  for (uint32_t i=0;i<128;i++)
    assert(input_block[i]==(float)(3000+i)-2048.0f);

  /* Ambos buffers de salida estan READY: no sobrescribirlos. */
  assert(output_blocks_prepared==2);
  for (uint32_t i=0;i<128;i++) feed(i==0?0:i==1?2048:4095);
  TP7_PrepareInputBlock();
  assert(input_blocks_prepared==2);
  assert(input_state[0]==TP7_BUFFER_READY);
  for (uint32_t i=0;i<TP7_STARTUP_SAMPLES;i++)
    assert(TP7_NextOutputSample()==2048);
  /* Reproducir el primero libera un slot para el tercer bloque. */
  for (uint32_t i=0;i<128;i++) check_code(TP7_NextOutputSample(),i);
  TP7_PrepareInputBlock();
  assert(input_block[0]==-2048.0f);
  assert(input_block[1]==0.0f);
  assert(input_block[2]==2047.0f);
  /* El tercer bloque NO debe adelantar al segundo aunque use el slot 0. */
  for (uint32_t i=0;i<128;i++) check_code(TP7_NextOutputSample(),128+i);
  for (uint32_t i=0;i<128;i++)
    check_code(TP7_NextOutputSample(),256+i);
  assert(output_blocks_played==3);
  if (setjmp(trap)==0) { TP7_NextOutputSample(); assert(0); }
  assert(sample_error==6);
  /* Preservar el estado previo de IRQ al salir de la seccion critica. */
  primask=1; TP7_PrepareInputBlock(); assert(primask==1); primask=0;

  /* Una IRQ ajena al ADC1 no debe cambiar los indices ni los contadores. */
  ADC_HandleTypeDef other={(void *)9};
  uint32_t before=adc_samples;
  HAL_ADC_ConvCpltCallback(&other); assert(adc_samples==before);

  /* Sin consumir el bloque anterior, detenerse antes de sobrescribirlo. */
  sample_error=0;
  for (uint32_t i=0;i<128;i++) feed(1234);
  for (uint32_t i=0;i<127;i++) feed(2345);
  if (setjmp(trap)==0) { feed(2345); assert(0); }
  assert(sample_error==5);
  for (uint32_t i=0;i<128;i++) assert(adc_buffers[1][i]==1234);

  /* Recorrido completo: timer -> DAC -> inicio ADC -> callback -> while.
   * Repetir con while inmediato y con un atraso cercano al limite de 16 ms. */
  TIM_HandleTypeDef timer={TIM8};
  for (uint32_t delayed=0;delayed<2;delayed++) {
    input_state[0]=TP7_BUFFER_FILLING; input_state[1]=TP7_BUFFER_FREE;
    output_state[0]=TP7_BUFFER_FREE; output_state[1]=TP7_BUFFER_FREE;
    adc_write_buffer=0; adc_write_index=0;
    dac_write_buffer=0; dac_read_buffer=0; dac_read_index=0;
    startup_remaining=TP7_STARTUP_SAMPLES;
    adc_pending=0; sample_error=0; timer_ticks=0;
    input_blocks_acquired=0; input_blocks_prepared=0;
    output_blocks_prepared=0; output_blocks_played=0;
    memset(fir_tail,0,sizeof(fir_tail));
    for (uint32_t n=0;n<4096;n++) reference_input[n]=(n*37+17)%4096;
    for (uint32_t n=0;n<4096;n++) {
      HAL_TIM_PeriodElapsedCallback(&timer);
      if (n<256) assert(last_dac==2048);
      else check_code(last_dac,n-256);
      assert(adc_pending==1);
      feed((n*37+17)%4096);
      if (!delayed || n%128==126) TP7_PrepareInputBlock();
    }
    assert(sample_error==0);
    assert(timer_ticks==4096);
    assert(output_blocks_played==30);
  }
  puts("OK: FIR float contra convolucion double, colas y fronteras, FIFO, errores y salida filtrada con 256 ticks de almacenamiento.");
  return 0;
}
'''

build = base / 'Tests/build'
build.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(dir=build) as temp:
    path = Path(temp)
    unit = path / 'buffers.c'
    unit.write_text(stubs + section('Includes') + '\n'.join(section(s) for s in
                    ['PTD', 'PD', 'PV', 'PFP', '0', '4']) + tests,
                    encoding='utf-8')
    exe = path / 'buffers.exe'
    subprocess.run(['gcc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(base / 'C'),
                    '-I' + str(base / 'MXCube/TP7/Core/Inc'),
                    str(unit), '-o', str(exe), '-lm'], check=True)
    subprocess.run([str(exe)], check=True)
