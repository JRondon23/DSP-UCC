%%Ejercicio gráfica de funcion cuadrada
pkg load signal

% Genero parametros de simulacion
  % Genero parametros de simulacion
  fs = 10000;          % Frecuencia de muestreo
  t_dur = 0.2;         % Duración de la simulación en segundos
  N1 = fs*t_dur;       % Cantidad de muestras
  n1 = 0:N1-1;         % Ancho de muestreo
  t1 = n1/fs;          % tiempo en base a la frecuencia de muestreo


 % Parametros para las funciones
 A_in=10;
 F_in=50;

 a = square(2*pi*F_in*t1, 50) * A_in; %El 50 determina el DC
 b = sawtooth(2*pi*F_in*t1,0.5)* A_in; %%EL 0.5 determina la relacion de ancho
 plot(t1,a)
 title('Señal Cuadrada')
 xlabel('t')
 ylabel('Amplitud')
 figure;
 plot(t1,b)
 title('Señal Triangular')
 xlabel('t')
 ylabel('amplitud')
