# TP7 en placa: paso actual

Implementacion sin DMA. Se avanza por etapas revisando la logica; se supone
que el hardware funciona. Ya hay buffers de adquisicion y preparacion del bloque
centrado, FIR temporal con superponer-sumar y reproduccion de bloques en el DAC.

## Paso 1: TIM8 inicia una muestra por periodo

Configuracion revisada: reloj de TIM8 de 160 MHz, PSC=19, ARR=999, RCR=0,
contador ascendente e interrupcion Update habilitada. Resultado: 8000 eventos
por segundo. TRGO permanece en Reset: ADC y DAC se controlan por software.

ADC1: PB14, canal 5, 12 bits, alineacion derecha, una conversion, inicio por
software, modo continuo desactivado y DMA desactivado. DAC1: PA5, canal 2,
12 bits, sin trigger, buffer de salida habilitado.

Input Capture en PC6 y las IRQ adicionales de TIM8 siguen en la configuracion
generada: el alumno decidio quitarlos desde CubeMX. No se inicia Input Capture
en el codigo de aplicacion; solo se inicia la base de tiempo de TIM8.

## Bloques de main.c

1. Constantes y variables: `TP7_DAC_MIDPOINT=2048` define el reposo del DAC.
   `dac_next_sample` es la muestra seleccionada por TIM8 para escribirla
   al DAC. `adc_pending` marca una adquisicion iniciada que todavia
   no termino de atenderse. `adc_last_sample` conserva la ultima lectura.
   Los contadores son auxiliares para comprender el flujo, no generan senales
   de prueba ni usan pines adicionales. `volatile` evita que el compilador
   suponga que estos valores no cambian entre el bucle principal y las IRQ;
   por si solo no resuelve la coordinacion de los futuros buffers.

2. Preparacion, en USER CODE 2: calibra el ADC, precarga el DAC en 2048 y
   habilita su canal. Habilita ADC1_2 en el NVIC desde codigo, porque no estaba
   habilitado en el .ioc. Reinicia el contador de TIM8, limpia su bandera de
   actualizacion e inicia TIM8 con interrupciones. No hay una espera de
   conversion dentro de la rutina de muestreo.

3. `HAL_TIM_PeriodElapsedCallback`: ignora los otros timers. Para TIM8,
   incrementa el contador, comprueba que no haya una adquisicion sin atender,
   escribe `dac_next_sample` en el DAC, marca el ADC como pendiente e inicia
   una conversion mediante `HAL_ADC_Start_IT`. La funcion regresa sin leer el
   resultado. En el paso 1 el DAC permanecia en 2048. En el paso 3 se agrega
   la seleccion de muestras del buffer de salida.

4. `HAL_ADC_ConvCpltCallback`: lee la conversion terminada, guarda la ultima
   muestra, incrementa su contador y borra la marca pendiente. No filtra ni
   escribe al DAC. Se agrego este minimo para que el arranque del ADC desde
   TIM8 tenga una atencion completa y coherente.

5. `HAL_ADC_ErrorCallback` y comprobaciones de estado: ante una conversion
   atrasada o un error de HAL, registran el motivo en `sample_error` y
   entran en el `Error_Handler` existente. No se imprime desde las IRQ.

6. `while(1)`: en el paso 2 atiende los bloques completos mediante
   `TP7_PrepareInputBlock`, como se describe a continuacion.

## Paso 2: adquisicion por bloques y entrega al while

Decisiones acordadas: dos buffers de entrada de 128 muestras, conversion a
float restando 2048 en el while, y detencion ante falta de un buffer libre.

Cada buffer sigue estos estados:

FREE -> FILLING -> READY -> PROCESSING -> FREE.

Al arrancar, A esta FILLING y B esta FREE. `HAL_ADC_ConvCpltCallback` guarda
cada lectura en A y avanza su indice de 0 a 127. Al completar 128 lecturas,
comprueba que B este FREE, publica A como READY y comienza a llenar B. Nunca
escribe en un buffer READY o PROCESSING.

`TP7_PrepareInputBlock`, llamada repetidamente desde el while, toma un buffer
READY y lo marca PROCESSING. Suspende las IRQ solo durante el cambio de
propiedad; guarda y restaura PRIMASK para respetar el estado anterior.
Despues copia las 128 muestras con las IRQ habilitadas:

`input_block[n] = (float)muestra_adc[n] - 2048.0f`.

Ejemplos: 0 -> -2048.0f; 2048 -> 0.0f; 4095 -> 2047.0f. Son cuentas ADC,
no voltios. El filtro futuro operara sobre este bloque centrado. Por ahora
se conserva el ultimo bloque, sin utilizarlo para generar una salida.

Cuando termina la copia, el buffer crudo pasa a FREE. `input_block` es
independiente: el ADC puede reutilizar el buffer crudo sin modificar la copia
que procesara el while. Si no hay ningun READY, la funcion regresa sin repetir
el bloque anterior ni alterar la copia. Las barreras DMB y los cambios cortos
de propiedad protegen la entrega entre la IRQ y el bucle principal.

Si al terminar B, A sigue READY o PROCESSING, se registra el codigo de error
5 y se entra en Error_Handler antes de reutilizar A. No se descarta un bloque
ni se sigue con una discontinuidad oculta. El while tiene aproximadamente
16 ms entre bloques para liberar el buffer anterior.

Los contadores `input_blocks_acquired` y `input_blocks_prepared`
permiten seguir cuantas entregas se completaron. El indice de escritura solo
lo modifica el callback del ADC; el while no lo reinicia ni lo comparte.

En el paso 2 la salida DAC seguia fija en 2048. El paso 3 amplia esta entrega
al while para producir y reproducir el bloque de salida sin filtrar.

## Paso 3: salida por bloques sin filtrar

Decisiones acordadas: salida sin filtro y retardo de almacenamiento de 256
muestras, equivalentes nominalmente a 32 ms a 8000 Hz.

Hay dos buffers de salida de 128 codigos DAC. Cada uno recorre:

FREE -> FILLING (while) -> READY -> PLAYING (TIM8) -> FREE.

`TP7_PrepareInputBlock` reserva a la vez una entrada READY y la salida FREE
que corresponde por orden. Si la salida todavia esta ocupada, regresa sin
consumir la entrada y reintenta en la siguiente vuelta. Convierte la entrada
a float centrado y libera el buffer crudo. Despues suma 2048, limita a 0..4095,
redondea al entero mas cercano y escribe los 128 codigos de salida. Sin filtro,
la resta y suma recuperan exactamente cada lectura ADC. Publica READY solo
al terminar las 128 escrituras, con una seccion critica corta.

`TP7_NextOutputSample` solo se llama desde TIM8. Durante los primeros 256 ticks
devuelve 2048. En el tick 257 empieza por la muestra 0 del primer bloque;
avanza una posicion por interrupcion. Cuando lee la posicion 127, libera ese
buffer y pasa al siguiente. La ultima muestra ya esta copiada en una variable
local antes de liberar el buffer: el while no puede alterar lo que se va a
escribir al DAC en ese tick.

Los indices de lectura y escritura alternan 0,1,0,1, en vez de buscar cualquier
salida READY. Asi, si el bloque 3 reutiliza el slot 0, no adelanta al bloque 2
que espera en el slot 1. Se mantiene el orden FIFO.

Si falta la salida READY esperada al comenzar un bloque, se registra el error
6 y se detiene el programa. No se repite una muestra o bloque anterior.
La espera inicial usa un contador que llega a cero y no vuelve a activarse
cuando el contador general de ticks se desborda.

En condiciones normales, la salida cumple y[n] = x[n-256], donde n es el indice
del tick y las primeras 256 salidas valen 2048. El margen entre un bloque de
entrada completo y su reproduccion prevista es aproximadamente de 16 ms.
Las IRQ solo atienden muestras individuales; la preparacion del bloque sigue
en el while. Todavia no se aplica convolucion FIR ni overlap-add.

`output_blocks_prepared` cuenta las salidas publicadas y
`output_blocks_played` cuenta los bloques reproducidos por completo.

## Recorrido entre las interrupciones

TIM8 Update -> TIM8_UP_IRQHandler -> HAL_TIM_IRQHandler
-> HAL_TIM_PeriodElapsedCallback -> escribir DAC -> iniciar ADC -> regresar.

ADC finaliza -> ADC1_2_IRQHandler -> HAL_ADC_IRQHandler
-> HAL_ADC_ConvCpltCallback -> guardar lectura -> liberar pending -> regresar.

Ambas IRQ tienen prioridad 0 y no se interrumpen entre si. La conversion es
asincrona: la muestra iniciada en un tick no es la salida emitida en ese tick.
El filtro entrega muestras de bloques adquiridos anteriormente.

## Integracion con CubeMX

Los cambios estan en zonas USER CODE de main.c y stm32g4xx_it.c/.h. El puente
manual `ADC1_2_IRQHandler` llama a HAL, que atiende y limpia los eventos del ADC.
Si mas adelante se habilita la IRQ ADC1_2 desde CubeMX y se regenera, CubeMX
creara su propio handler: quitar la definicion manual para que quede una sola.

## Revision de este paso

En el paso 1 se validaron main.c y stm32g4xx_it.c con el compilador ARM de STM32CubeIDE,
para Cortex-M4 y las cabeceras reales del proyecto, sin errores ni advertencias
en la comprobacion de sintaxis. Esto no es una prueba de ejecucion en placa.

Para el paso 2 se ejecuto `Tests/verificar_buffers.py`, que extrae los bloques
de codigo reales de main.c y los prueba con HAL simulada en PC. Verifica las
fronteras de 128 muestras, la alternancia durante 20 bloques adicionales, el
orden de las lecturas, el centrado, la entrega unica, la restauracion de IRQ y
la detencion por sobrecarga sin sobrescritura. No requiere conectar la placa.
La solicitud para volver a ejecutar el compilador ARM fue rechazada. Luego
el alumno confirmo que el paso 2 compila correctamente desde STM32CubeIDE.

Para el paso 3 se amplio el mismo test: valida el orden FIFO aun cuando se
reutiliza el slot 0 antes de reproducir el slot 1, la reserva de salidas, el
error 6 y dos recorridos completos de 4096 ticks. Uno prepara bloques de
inmediato y otro demora el while hasta cerca del limite de 16 ms. Se comprueba
cada codigo escrito al DAC y un retardo exacto de 256 muestras. Las pruebas
pasaron. Queda compilar este nuevo paso desde STM32CubeIDE.

Antes de continuar: revisar que la salida se publique solo al completar sus
128 posiciones y que TIM8 avance una muestra por tick. El siguiente paso es
revisar el FIR temporal del paso 4 conservando esta coordinacion de entrada y salida.

## Paso 4: FIR temporal con superponer-sumar

Se acordo usar float y los mismos 129 coeficientes del diseno de 200 Hz.
`Core/Inc/tp7_fir_coeffs.h` contiene esos coeficientes con sufijo f para que
las operaciones sean de precision simple. El script `Octave/disenar_filtro.m`
ahora exporta tambien este encabezado cuando existe el proyecto de placa.
No se cambia el filtro de PC ni se introduce una nueva especificacion.

La funcion `TP7_FilterInputBlock` se llama desde `TP7_PrepareInputBlock`,
en el while, despues de centrar la entrada y liberar el buffer ADC crudo.
Las IRQ siguen habilitadas durante todo el calculo. TIM8 solo reproduce
muestras ya preparadas y solicita conversiones ADC, como en el paso anterior.

El calculo tiene cuatro partes comentadas en main.c:

1. Poner en cero las 256 posiciones del resultado del bloque actual.
2. Acumular `resultado[n+k] += entrada[n] * h[k]`, con n=0..127 y k=0..128.
3. Sumar la cola del bloque anterior a las primeras 128 posiciones.
4. Guardar las ultimas 128 posiciones como nueva cola.

La cola no se borra al comenzar un bloque: representa contribuciones de
entradas pasadas a salidas futuras. Sin ella, apareceria una discontinuidad
de procesamiento cada 128 muestras. Se inicializa en cero al arrancar, de
modo que las condiciones iniciales del FIR sean nulas.

El while convierte las primeras 128 salidas centradas a codigos DAC sumando
2048, limitando al rango 0..4095 y redondeando. Nunca aplica esa limitacion
a la cola: la memoria del filtro permanece en float sin saturacion DAC.
Solo publica el bloque READY despues de completar todo el procesamiento.

El filtro es simetrico y tiene un retardo de grupo de (129-1)/2=64 muestras,
equivalentes a 8 ms. Se mantienen los 32 ms del almacenamiento: el retardo
de grupo total nominal en banda es 40 ms. Esto no agrega otra espera de 8 ms
en el codigo: los 8 ms son una propiedad de la fase del FIR. El transitorio
inicial depende tambien de sus condiciones iniciales.

El corte nominal sigue siendo aproximadamente -6,02857 dB en 200 Hz. La
conversion de coeficientes a float cambia cada coeficiente menos de 1,84e-9.
El rechazo del diseno desde 350 Hz sigue superando los 40 dB requeridos.

La convolucion y la cola usan arreglos estaticos, no arreglos grandes sobre
la pila. Hay 256 floats para resultado y 128 para cola: 1536 bytes adicionales
de RAM. El trabajo por bloque es de 128*129=16512 productos acumulados.
La frecuencia de corte, la cantidad de coeficientes y el tamano de bloque
estan identificados en constantes; comprobaciones de preprocesador impiden
compilar una frecuencia de diseno incompatible o una cola mayor al bloque.

Se actualizo `Tests/verificar_buffers.py` para ejecutar el codigo real de
main.c con HAL simulada y comparar el FIR float con una convolucion directa
independiente en double, usando el encabezado original de PC como referencia.
Las pruebas cubren impulsos en las muestras 127 y 128, continua, ruido y
una mezcla de 100, 1000 y 2000 Hz, ademas de vaciar toda la cola con ceros.
Se exige error menor de 0,01 cuentas ADC antes de redondear y como maximo
una cuenta DAC de diferencia frente a la referencia redondeada. Se mantienen
las pruebas de orden FIFO, errores y ambos recorridos de 4096 ticks con
atencion inmediata o demorada del while. Todas pasaron.

El alumno confirmo la compilacion del paso 3. Queda compilar el paso 4 desde
STM32CubeIDE. No se realizo una medicion de tiempo de ejecucion en placa.
