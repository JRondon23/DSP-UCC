% 9) Genere una sinusoidal de 1 kHz con una frecuencia de muestreo de 10 kHz y
% 1000 muestras. Calcule su FFT y grafique el espectro de amplitud.


%%Ejercicio gráfica de seno


% Genero parametros de simulacion
  % Genero parametros de simulacion
  fs = 10000;          % Frecuencia de muestreo
  t_dur = 0.1;         % Duración de la simulación en segundos
  N1 = fs*t_dur;       % Cantidad de muestras
  n1 = 0:N1-1;         % Ancho de muestreo
  t1 = n1/fs;          % tiempo en base a la frecuencia de muestreo


 % Parametros para el seno
 A_in=1;
 F_in=1*10^3;


 x = Seno_function(A_in,F_in,t1);


 L=length(x); %Obtengo el tamaño de la señal X.
 T=1/fs; %Obtengo el periodo de muestreo
 t=(0:L-1)*T; %Obtengo el tiempo
 y = fft(x); %Hago la FFT
 P2=abs(y/L); %Lo normalizo por el modo en que funciona FFT
 P1 = P2(1:L/2+1); %Obtengo solamente la mitad positiva
 P1(2:end-1) = 2*P1(2:end-1); %Lo que hago es no perder la energia de la parte negativa agregandoselo a todo lo de la parte positiva excepto al DC y Fs/2(Nyquist)
 f = fs*(0:(L/2))/L; % vector de frecuencias.
 figure;
 plot(f,P1); %Plotea P1
 figure;
 plot(f,20*log10(P1)) % grafica con salida en dBs.
 title('Amplitud en el dominio de la frecuencia')
 xlabel('f (Hz)')
 ylabel('|P1(f)|')





