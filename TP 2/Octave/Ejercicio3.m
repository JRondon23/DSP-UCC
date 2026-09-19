% EJERCICIO 3

  fs = 100;   % Frecuencia de muestreo
  fs_2 = 2000;
  t_dur = 1;         % Duración de la simulación en segundos
  N1 = fs*t_dur;  % Cantidad de muestras
  N2 = fs_2*t_dur;
  n1 = 0:N1-1;  % Ancho de simulacion
  n2 = 0:N2-1;
  t1 = n1/fs;       % Cada cuanto muestrea
  t2 = n2/fs_2;

  F_a=25;
  F_b=70;
  F_c=160;
  F_d=510;

  a = sin(2*pi*F_a*t1);
  b = sin(2*pi*F_b*t1);
  c = sin(2*pi*F_c*t1);
  d = sin(2*pi*F_d*t1);

  X = a+b+c+d;

  a_2 = sin(2*pi*F_a*t2);
  b_2 = sin(2*pi*F_b*t2);
  c_2 = sin(2*pi*F_c*t2);
  d_2 = sin(2*pi*F_d*t2);

  Y = a_2+b_2+c_2+d_2;

  stem(t1,X);
  xlabel('n');
  ylabel('Amplitud');
  title('Suma de senos incorrecto');
  figure;

  plot(t2,Y);
  xlabel('n');
  ylabel('Amplitud');
  title('Suma de senos correcto');
  figure;


  %%FALTA HACER LA TRANSFORMADA PARA COMPROBAR QUE COMPONENTES TIENE

  %FFT de Seno incorrecto
 L=length(X); %Obtengo el tamaño de la señal X.
 T=1/fs; %Obtengo el periodo de muestreo
 t=(0:L-1)*T; %Obtengo el tiempo
 Z = fft(X); %Hago la FFT
 P2=abs(Z/L); %Lo normalizo por el modo en que funciona FFT
 P1 = P2(1:L/2+1); %Obtengo solamente la mitad positiva
 P1(2:end-1) = 2*P1(2:end-1); %Lo que hago es no perder la energia de la parte negativa agregandoselo a todo lo de la parte positiva excepto al DC y Fs/2(Nyquist)
 f = fs*(0:(L/2))/L; % vector de frecuencias.
 plot(f,P1); %Plotea P1
 figure;


  %FFT de Seno Correcto
 L_C=length(Y); %Obtengo el tamaño de la señal X.
 T_C=1/fs_2; %Obtengo el periodo de muestreo
 t_C=(0:L_C-1)*T_C; %Obtengo el tiempo
 Z_C = fft(Y); %Hago la FFT
 P2_C=abs(Z_C/L_C); %Lo normalizo por el modo en que funciona FFT
 P1_C = P2_C(1:L_C/2+1); %Obtengo solamente la mitad positiva
 P1_C(2:end-1) = 2*P1_C(2:end-1); %Lo que hago es no perder la energia de la parte negativa agregandoselo a todo lo de la parte positiva excepto al DC y Fs/2(Nyquist)
 f_C = fs_2*(0:(L_C/2))/L_C; % vector de frecuencias.
 plot(f_C,P1_C); %Plotea P1
 figure;
