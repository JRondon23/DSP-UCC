% 5) Utilice el ambiente de simulación para
% generar un archivo de cabecera (archivo con extensión “.h”)
% con las muestras almacenadas de una señal.

% Crea un archivo en C que contiene un array con los valores de una seno, puede ser util para hacerlo correr en un DAC con la finalidad de obtener un seno a la salida

 %#forma típica de exportar una señal generada/procesada en Octave hacia un proyecto embebido en C


 % Genero parametros de simulacion
 % Genero parametros de simulacion
 fs = 2000;          % Frecuencia de muestreo
 t_dur = 0.2;         % Duración de la simulación en segundos
 N1 = fs*t_dur;       % Cantidad de muestras
 n1 = 0:N1-1;         % Ancho de muestreo
 t1 = n1/fs;          % tiempo en base a la frecuencia de muestreo


 % Parametros para el seno
 A_in=1;
 F_in=50;


 Grafica = Seno_function(A_in,F_in,t1);


 array = Grafica; %Le pone nombre array a grafica
 min_index = 1; %Indica el minimo
 max_index = 100; %indica el maximo
 file_name = 'test_signal.h'; %Nombre del archivo
 const_name = 'ELEMENTS'; %Nombre que le va a poner a la constante
 array_name = ['short test_signal[' const_name ']={'];  %Nombre del arreglo con el nombre de la constante

 % crea el archivo de cabecera
 fid=fopen(file_name,'w'); %Indica que le va a escribir utilizando fid, lo crea y indica que escribe
 fprintf(fid,'#define '); %Escribe #define
 fprintf(fid, const_name); %Escribe a la derecha de #Define el nombre de la constante
 fprintf(fid, ' %d\n\n', max_index); %Agrega 100.
 % #DEFINE ELEMENTS 100;

 fprintf(fid, array_name); %Define el arreglo: short test_signal[ELEMENTS]
 fprintf(fid,'%d, ' , array(min_index:(max_index-1))); %Coloca los primeros 99 valores seguidos de ,
 fprintf(fid,'%d' , array(max_index));  %Coloca el valor final
 fprintf(fid,'};\n');
 fclose(fid);

