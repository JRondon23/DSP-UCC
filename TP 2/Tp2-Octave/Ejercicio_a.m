
%EJERCICIO 1:

  %a

  fs = 0.1;          % Frecuencia de muestreo
  t_dur = 300;         % Duración de la simulación en segundos
  N1 = fs*t_dur;       % Cantidad de muestras
  n1 = 0:200;         % Ancho de simulacion
  t1 = n1/fs;          % Cada cuanto muestrea

  F_in=0.005; %valor en herzs

  X = cos(2*pi*F_in*t1);

  stem(t1,X);
  xlabel('Tiempo [s]');
  ylabel('Amplitud');
  title('Coseno Discreto');


  %b

  fs = 10;          % Frecuencia de muestreo
  t_dur = 300;         % Duración de la simulación en segundos
  N1 = fs*t_dur;       % Cantidad de muestras
  n1 = 0:N1-1;         % Ancho de simulacion
  t1 = n1/fs;          % Cada cuanto muestrea

  F_in=(30/105); %valor en herzs

  X = cos(pi*F_in*t1);

  stem(t1,X);
  xlabel('Tiempo [s]');
  ylabel('Amplitud');
  title('Coseno Discreto');

  %c

  fs = 150;          % Frecuencia de muestreo
  t_dur = 3;         % Duración de la simulación en segundos
  N1 = fs*t_dur;       % Cantidad de muestras
  n1 = 0:N1-1;         % Ancho de simulacion
  t1 = n1/fs;          % Cada cuanto muestrea

  F_in=3; %valor en herzs

  X = cos(pi*F_in*t1);

  stem(t1,X);
  xlabel('Tiempo [s]');
  ylabel('Amplitud');
  title('Coseno Discreto');

  %d

  fs = 15;          % Frecuencia de muestreo
  t_dur = 3;         % Duración de la simulación en segundos
  N1 = fs*t_dur;       % Cantidad de muestras
  n1 = 0:N1-1;         % Ancho de simulacion
  t1 = n1/fs;          % Cada cuanto muestrea

  F_in_d=3; %valor en herzs

  X = sin(F_in_d*t1);

  stem(t1,X);
  xlabel('Tiempo [s]');
  ylabel('Amplitud');
  title('Seno Discreto');

  %e

  fs = 24.8;          % Frecuencia de muestreo
  t_dur = 3;         % Duración de la simulación en segundos
  N1 = fs*t_dur;       % Cantidad de muestras
  n1 = 0:N1-1;         % Ancho de simulacion
  t1 = n1/fs;          % Cada cuanto muestrea

  F_in_d=(62/10); %valor en herzs

  X = sin(pi*F_in_d*n1);

  stem(n1,X);
  xlabel('Tiempo [s]');
  ylabel('Amplitud');
  title('Seno Discreto');

