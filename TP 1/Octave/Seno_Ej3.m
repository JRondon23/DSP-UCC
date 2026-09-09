%Defino parametros
  A_1 = 1;
  A_2 = 1;
  F_1 = 50;
  F_2 = 30;

% Genero parametros de simulacion
  % Genero parametros de simulacion
  fs = 2000;           # Frecuencia de muestreo
  t_dur = 0.2;         # Duración de la simulación en segundos
  N1 = fs*t_dur;       # Cantidad de muestras
  n1 = 0:N1-1;
  t1 = n1/fs;


% Suma de dos senos con misma amplitu y distinta frecuencia.

  X=Seno_function(A_1,F_1,t1);
  Y=Seno_function(A_2,F_2,t1);

% Realizo la sumar

  Suma_senos=X+Y;


  plot(t1, Suma_senos)
  xlabel('t (s)')
  ylabel('Amplitud')
  title(['Suma de senos - fs = ' num2str(fs) ' Hz'])
  grid on
