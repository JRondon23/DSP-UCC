# TP7 - Procesamiento por bloques

Resolucion en PC con C y GNU Octave. No incluye firmware ni mediciones fisicas.

## Como funcionan los programas

`ejecutar_tp7.m` es el menu: coordina el diseno, la compilacion, las ejecuciones y la visualizacion. Octave disena y analiza; los ejecutables C realizan el filtrado.

`Octave/disenar_filtro.m` calcula 129 coeficientes de un FIR pasabajos con ventana Kaiser y corte nominal de 200 Hz (aproximadamente -6 dB). Normaliza la ganancia en continua, verifica su respuesta y exporta los coeficientes a `C/filter.h`. El retardo propio del filtro es 64 muestras, es decir, 8 ms a 8000 Hz.

`C/timeconv.c` selecciona la convolucion temporal (`USE_FFT=0`). `C/fastconv_rt.c` selecciona la convolucion por FFT (`USE_FFT=1`). Ambos incluyen `C/simulacion.h`, donde esta el codigo comun. Por eso los dos archivos principales son cortos.

Cada ejecutable genera ruido reproducible, una suma de senoidales de 100, 1000 y 2000 Hz, o un impulso. Divide la entrada en bloques de 128 muestras. En la version temporal, multiplica cada muestra por los coeficientes y acumula los productos: calcula directamente la convolucion.

En la version FFT, transforma el filtro una sola vez. Para cada bloque, agrega ceros hasta 256 puntos, calcula la FFT, multiplica por la FFT del filtro y aplica la transformada inversa. El resultado es la misma convolucion lineal porque 256 = 128 + 129 - 1.

Ambas versiones usan superponer-sumar (overlap-add): cada bloque produce 256 valores; se suman los primeros 128 con la cola anterior, se emiten y se guardan los otros 128 para el bloque siguiente. Al finalizar se agregan ceros para terminar toda la respuesta. Cada CSV contiene numero de muestra, tiempo, entrada y salida.

`Octave/verificar_y_graficar.m` compara los CSV con la funcion `conv` de Octave y entre si. Tambien prueba entradas mas cortas y bloques incompletos. Luego grafica las senales en el tiempo y sus espectros por FFT. La FFT del grafico analiza las frecuencias presentes; la FFT del programa C es parte del algoritmo de filtrado.

Todas las curvas son rojas. En la comparacion temporal, la linea continua corresponde al metodo temporal y la discontinua al metodo FFT. En los espectros, la continua representa la entrada y la discontinua la salida. Para ver las imagenes actualizadas desde el menu, cerrar las ventanas anteriores y elegir el paso 6.

Las figuras incluyen cuadricula, escalas numericas, etiquetas con unidades y leyendas dentro de cada eje. `Octave/formato_grafica.m` fija explicitamente los colores y el formato para que se vean tanto en Octave como en los PNG exportados.

## Carpetas

- `Consigna/`: PDF recibido.
- `C/`: `timeconv.c` (convolucion temporal) y `fastconv_rt.c` (FFT en PC), con implementacion comun en `simulacion.h` y coeficientes en `filter.h`.
- `C/Originales/`: copia de los tres programas recibidos, sin modificaciones. `C/timeconv_rt.c` tambien se conserva como referencia; no forma parte de la compilacion.
- `Octave/`: diseno, comparacion con `conv` y generacion de graficas.
- `Tests/`: ejecucion reproducible de todo el trabajo.
- `Resultados/`: coeficientes, senales CSV, graficas y verificaciones numericas.
- `Informe/`: desarrollo, resultados y conclusiones.

## Ejecutar

### Desde la interfaz de Octave, sin escribir comandos

1. Abrir GNU Octave con su interfaz grafica.
2. Elegir Archivo > Abrir y seleccionar `TP 7/ejecutar_tp7.m`.
3. Pulsar Ejecutar (F5). Si pregunta, elegir cambiar al directorio del archivo.
4. En el menu, realizar los pasos 1 a 5 en orden; esperar el aviso de finalizacion de cada uno.
5. Elegir el paso 6, Ver graficas en Windows. Se abre la carpeta Resultados; hacer doble clic en un PNG para verlo con el visor de imagenes de Windows. El menu termina automaticamente para evitar bloqueos.

El menu ejecuta las herramientas necesarias desde Octave. No utiliza PowerShell ni requiere cambiar sus permisos. Si se modifica el diseno, repetir los pasos 1 a 5 para actualizar todos los resultados.

### Ejecucion automatica opcional

En PowerShell, desde la raiz del repositorio:

```powershell
& './TP 7/Tests/ejecutar.ps1'
```

Requiere GNU Octave instalado en `Program Files/GNU Octave`, el paquete `signal`, GCC y gnuplot incluidos en esa distribucion. El script genera el filtro antes de compilar, ejecuta las dos versiones y verifica los resultados. Los ejecutables quedan en `C/build/`.

Para ejecutar un programa ya compilado, usar el entorno de Octave (sus DLL deben estar en PATH):

```powershell
& './TP 7/C/build/timeconv.exe' ruido './TP 7/Resultados/otra_prueba.csv' 8192
& './TP 7/C/build/fastconv_rt.exe' senos './TP 7/Resultados/otra_fft.csv' 8192
```

Ambos aceptan `ruido`, `senos` o `impulso`, ruta CSV y cantidad opcional de muestras. Los CSV tienen columnas `n,t_s,x,y`; incluyen 128 muestras adicionales para terminar la respuesta del filtro. La simulacion no intenta cumplir plazos de tiempo real.

La frecuencia de corte nominal es 200 Hz (aproximadamente -6 dB, por el metodo de ventana). Se verifica paso de 0 a 100 Hz con menos de 1 dB de perdida y rechazo desde 350 Hz con al menos 40 dB. La respuesta ya no se exige plana hasta 200 Hz. Para cambiar el diseno, editar `Octave/disenar_filtro.m` y repetir la ejecucion completa. Si se cambia la cantidad de coeficientes, ajustar tambien los tamanos de bloque y FFT respetando `FFT_SIZE >= BLOCK + N - 1`.

