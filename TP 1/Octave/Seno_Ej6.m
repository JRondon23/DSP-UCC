%Ejercicio gráfica de ruido


% Genero parametros de simulacion
  % Genero parametros de simulacion
  fs = 2000;          % Frecuencia de muestreo
  t_dur = 0.2;         % Duración de la simulación en segundos
  N1 = fs*t_dur;       % Cantidad de muestras
  n1 = 0:N1-1;         % Ancho de muestreo
  t1 = n1/fs;          % tiempo en base a la frecuencia de muestreo


 Grafica = rand(1, N1)*e^-10; % Hago random de una fila y de N1 muestras, luego lo atenuo.


 % Ploteo gráfica
 plot(t1, Grafica)
  xlabel('t (s)')
  ylabel('Amplitud')
  title(['Ejercicio Grafica Seno'])
  grid on



