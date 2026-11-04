# TP6: dos FIR y dos IIR

El proyecto es `STM32/TP5-DSP/TP5-DSP.ioc`, para STM32G491RE.
Se conservan los coeficientes de los FIR pasabajo y pasaalto. Los dos IIR
implementados son pasabanda y rechazabanda, siguiendo las bandas que estaban
intentando implementar. No se modificaron los archivos originales de TP5.

## Contexto y decisiones de diseño

TP5 pide diseñar y comparar filtros en Octave y C. Su anexo especifica secciones
de segundo orden en cascada. TP6 lleva esos filtros a la placa y pide medir su
ganancia con generador y osciloscopio para compararla con el diseño.
Se aplica la reducción indicada por el profesor: dos FIR y dos IIR.

Todos usan fs = 8000 Hz. Para precisar las transiciones que el enunciado no fija,
se eligieron estas especificaciones para los IIR:

| Filtro | Banda de paso | Banda de rechazo | Transiciones |
| --- | --- | --- | --- |
| Pasabanda | 200–300 Hz | 0–150 Hz y 350–4000 Hz | 150–200 Hz y 300–350 Hz |
| Rechazabanda | 0–200 Hz y 700–4000 Hz | 300–500 Hz | 200–300 Hz y 500–700 Hz |

Pérdida máxima en paso: 1 dB. Atenuación mínima en rechazo: 40 dB.
Ambos son Chebyshev II. `cheb2ord` calcula el orden necesario:
pasabanda de orden total 12 (6 secciones) y rechazabanda de orden total 8
(4 secciones). Un orden fijo de 3 no garantiza estas especificaciones.
En los filtros de banda, el orden total es el doble del orden del prototipo.
En Chebyshev II, los bordes entregados a `cheby2` son bordes de rechazo;
no deben confundirse con bordes de paso o puntos de -3 dB.

Si el profesor pide otros tipos de IIR o transiciones distintas, ajustar `wp`,
`ws`, `rp` y `rs` en `Octave/disenar_iir.m` y volver a generarlos.

## Qué fallaba

1. En `Core/Inc/FIR.h`, los vectores denominados `b_*` contenían denominadores
   y los denominados `a_*` contenían numeradores. Además, los vectores llamados
   pasabanda tenían una estructura de numerador simétrico compatible con un
   rechazo de banda; sus nombres no permitían confiar en el diseño utilizado.
2. El bucle `for (int i=N_iir-1; i < 0; --i)` nunca se ejecutaba. El historial
   de salidas no se actualizaba.
3. La realimentación empezaba en el índice cero, incluyendo `a[0]`. En una
   implementación directa la ecuación correcta es:

   `y[n] = (sum(b[k]*x[n-k], k=0..M) - sum(a[k]*y[n-k], k=1..N)) / a[0]`

   Si el historial guarda `y[n-1]` en la posición cero, `a[1]` multiplica esa
   posición. No se debe multiplicar `a[0]` por una salida pasada.
4. Exportar polinomios de orden alto con seis decimales y evaluarlos en `float`
   agrava los errores numéricos, especialmente en bandas estrechas. Ahora se
   diseñan ceros y polos y se convierten directamente a SOS.
5. La entrada se convertía a voltios sin quitar su offset y se añadía 1 V a la
   salida. Ahora se resta el offset de entrada y se suma el de salida de forma
   explícita. La señal filtrada interna puede ser positiva o negativa.
6. La conversión a `uint32_t` ocurría antes de limitar la salida. Ahora la
   limitación se hace en `float` entre 0 y 3.3 V, antes de convertir.
7. El callback modificaba el buffer mientras el bucle principal lo leía, usando
   una bandera que podía agrupar o perder muestras. Ahora cada conversión ejecuta
   una vez el filtro y actualiza el DAC dentro del callback.

Las tablas IIR antiguas se retiraron de `FIR.h`. `TP 6/C/coeficientes.h` y
`TP 5/Octave/IIR/IIR.m` son archivos anteriores: no alimentan el firmware nuevo.

## Cómo funciona la implementación nueva

`Core/Inc/iir_coeffs.h` contiene ambas matrices, generadas por Octave.
Cada fila tiene `[b0 b1 b2 1 a1 a2]`. Se exportan nueve cifras significativas
de los coeficientes cuantizados a float32; la ganancia ya está incluida.
No se deben invertir los signos al copiarlos.

`Core/Inc/iir_sos.h` aplica la forma directa II transpuesta. Cada sección guarda
dos estados, inicialmente cero:

```text
y       = b0*x + z1
z1_nuevo = b1*x - a1*y + z2
z2_nuevo = b2*x - a2*y
```

La salida de una sección es la entrada de la siguiente. Los estados persisten
entre muestras. No se reinician en cada interrupción ni se limitan al rango del
DAC. El recorte corresponde únicamente a la salida analógica final.
No se necesita CMSIS-DSP: el código utiliza C y las funciones HAL existentes.

En `main.c` se calibra el ADC, se prepara el DAC y después se inicia TIM2.
El reloj actual es 160 MHz y TIM2 tiene PSC=1 y ARR=9999:

`fs = 160000000 / ((1+1)*(9999+1)) = 8000 Hz`

El período de muestra es 125 us. No hay que cambiar la configuración actual
del temporizador. El disparo del ADC sigue siendo por software desde TIM2;
si luego se requiere menor variación temporal puede migrarse al disparo hardware.

Corrección de la banda desplazada observada en la placa: se encontró ARR=999
en `tim.c` y en el `.ioc`, que solicita 80000 Hz con este reloj y prescaler.
Se restableció ARR=9999 en ambos archivos. Los coeficientes siguen diseñados
para 8000 Hz. Octave ahora comprueba el prescaler y período en ambos archivos
y la frecuencia del temporizador registrada en CubeMX antes de exportarlos.
Esta comprobación no sustituye medir el reloj y la tasa efectiva en la placa
si se modifica manualmente `SystemClock_Config` o se pierden muestras.
Los 200 y 300 Hz son bordes de paso de -1 dB; los cortes de -3 dB están
ligeramente por fuera y se imprimen al ejecutar el diseño.

## Usarlo en STM32CubeIDE

1. Abrir el proyecto existente `STM32/TP5-DSP` en CubeIDE. CubeMX configura los
   periféricos; CubeIDE compila y permite cargar el firmware.
2. Elegir el filtro en `Core/Src/main.c`, dentro de `USER CODE BEGIN PD`:

   ```c
   #define FILTRO_ACTIVO FILTRO_IIR_PASABANDA
   ```

   Las otras opciones son `FILTRO_IIR_RECHAZABANDA`, `FILTRO_FIR_PASABAJO` y
   `FILTRO_FIR_PASAALTO`. Se selecciona uno por compilación; reiniciar la placa
   al cargar el nuevo firmware deja todos los estados en cero.
3. Compilar y cargar. Los cambios de `main.c` están en bloques `USER CODE`,
   para conservarlos al regenerar desde CubeMX.
4. Entrada en PA0 (ADC1_IN1), salida en PA4 (DAC1_OUT1), masa común.
5. Para empezar, usar una sinusoidal de 1 Vpp con offset de 1.65 V. La entrada
   debe quedar dentro de 0–3.3 V. Los valores `OFFSET_ENTRADA_V` y
   `OFFSET_SALIDA_V` están configurados en 1.65 V; ajustar el primero si el
   offset real del generador es otro. `ADC_VREF` debe representar la referencia
   real si se necesita precisión absoluta de tensión.
6. Medir la amplitud de entrada en PA0 y la de salida en PA4, usando Vpp en
   ambas, después de que termine el transitorio. Calcular:

   `Ganancia_dB = 20*log10(Vpp_salida / Vpp_entrada)`

   La comparación es sobre la componente alterna. El nivel continuo del DAC es
   el offset configurado y no es la amplitud de salida del filtro.

Durante la prueba, observar en el depurador:

- `conv`: salida filtrada antes del offset y la limitación del DAC.
- `muestras_procesadas`: aumenta unas 8000 veces por segundo, sin detener la CPU.
- `errores_adc` y `fallos_numericos`: deben permanecer en cero.
- `saturaciones_dac`: debe permanecer en cero en régimen con la amplitud propuesta.
  Si aumenta, reducir la amplitud; medir una señal recortada da una respuesta incorrecta.
- `ciclos_filtro_max`: máximo de ciclos del callback de conversión. A 160 MHz,
  dividir por 160 para obtener us. Debe dejar margen respecto de los 20000 ciclos
  disponibles por muestra, incluyendo además la conversión, los otros callbacks
  y las entradas/salidas de interrupción. El valor real debe medirse en la placa.

## Octave: generar y visualizar

Desde la carpeta `TP 6/Octave`, ejecutar:

```octave
disenar_iir();
```

Esto requiere el paquete `signal`. Genera automáticamente el encabezado en
`STM32/TP5-DSP/Core/Inc/iir_coeffs.h`: no hace falta copiar coeficientes.
Valida estabilidad de cada sección y especificaciones en una malla densa,
incluidos los bordes exactos, después de cuantizar a `float`.
Muestra amplitud y fase y guarda los diseños `.mat` para la comparación con C.
Los archivos `*_respuesta.csv` tienen dos columnas sin cabecera: Hz y ganancia
en dB. Sirven para superponer la curva de diseño y las mediciones del informe.
Después de regenerar coeficientes, recompilar en CubeIDE.

`zp2sos` se llama con una salida para incorporar la ganancia a la primera
sección, según la [documentación de Octave](https://gnu-octave.github.io/octave-signal//functions/6_zp2sos/).

## Verificación reproducible del C

Desde PowerShell, en la raíz del repositorio:

```powershell
& './TP 6/Tests/verificar.ps1'
```

El script encuentra GNU Octave instalado en `Program Files`, utiliza su GCC
para compilar `Tests/iir_sim.c`, regenera los diseños y compara el mismo núcleo C
del firmware contra `filter` de Octave sección por sección.
Se prueban impulso, escalón, multitono y ruido de 10 segundos por filtro.
La tolerancia es un error absoluto máximo de 0.001 para estas señales de prueba.
El simulador reinicia sus estados para cada entrada.

Para ver las gráficas de entrada/salida en tiempo y FFT, después de esa prueba:

```octave
verificar_iir();
```

Resultados obtenidos en esta computadora: ocho comparaciones aprobadas;
error absoluto máximo 4.01e-6. Compilación y enlace del firmware predeterminado
(IIR pasabanda) aprobados con el compilador ARM de STM32CubeIDE.
La prueba de escritorio evalúa la aritmética `float` del C; las mediciones
analógicas, el tiempo real y el comportamiento de la placa quedan por verificar
en laboratorio.

## Frecuencias sugeridas para el laboratorio

| Filtro | Paso | Bordes de paso | Transición | Bordes de rechazo | Rechazo |
| --- | --- | --- | --- | --- | --- |
| Pasabanda | 250 Hz | 200 y 300 Hz | 175 y 325 Hz | 150 y 350 Hz | 100, 400 y 1000 Hz |
| Rechazabanda | 100 y 1000 Hz | 200 y 700 Hz | 250 y 600 Hz | 300 y 500 Hz | 350, 400 y 450 Hz |

Ganancias medidas en la simulación del C con multitono (sin ADC/DAC):

| Frecuencia | Pasabanda | Rechazabanda |
| --- | --- | --- |
| 100 Hz | -50.31 dB | aproximadamente 0 dB |
| 250 Hz | aproximadamente 0 dB | -8.77 dB (transición) |
| 400 Hz | -43.27 dB | -40.95 dB |
| 1000 Hz | -44.56 dB | -0.008 dB |

Para el informe, registrar frecuencia, Vpp de entrada, Vpp de salida y ganancia;
comparar con el CSV de diseño y adjuntar capturas del osciloscopio. Con rechazo
de 40 dB, una entrada de 1 Vpp produce unos 10 mVpp o menos: el ruido de la placa
y la resolución de medición pueden condicionar ese resultado.
