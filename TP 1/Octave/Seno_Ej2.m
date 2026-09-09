%Ejercicio gráfica de seno


% Genero parametros de simulacion
  % Genero parametros de simulacion
  fs = 2000;          % Frecuencia de muestreo
  t_dur = 0.2;         % Duración de la simulación en segundos
  N1 = fs*t_dur;       % Cantidad de muestras
  n1 = 0:N1-1;         % Ancho de muestreo
  t1 = n1/fs;          % tiempo en base a la frecuencia de muestreo


 % Parametros para el seno
 A_in=5;
 F_in=50;


 Grafica = Seno_function(A_in,F_in,t1);


 % Ploteo gráfica
 plot(t1, Grafica)
  xlabel('t (s)')
  ylabel('Amplitud')
  title(['Ejercicio Grafica Seno'])
  grid on



