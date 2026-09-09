%%Ejercicio gráfica de seno


% Genero parametros de simulacion
  % Genero parametros de simulacion
  fs = 10000;          % Frecuencia de muestreo
  t_dur = 0.2;         % Duración de la simulación en segundos
  N1 = fs*t_dur;       % Cantidad de muestras
  n1 = 0:N1-1;         % Ancho de muestreo
  t1 = n1/fs;          % tiempo en base a la frecuencia de muestreo


 % Parametros para el seno
 A_in=10;
 F_in=1000;

 a = Seno_function(A_in,F_in,t1);
 b = rand(1, N1)*e^-2;

 x = a+b;

 %Grafico sin ruido
 La=length(a); %Obtengo el tamaño de la señal X.
 Ta=1/fs; %Obtengo el periodo de muestreo
 t=(0:La-1)*Ta; %Obtengo el tiempo
 ya = fft(a); %Hago la FFT
 P2a=abs(ya/La); %Lo normalizo por el modo en que funciona FFT
 P1a = P2a(1:La/2+1); %Obtengo solamente la mitad positiva
 P1a(2:end-1) = 2*P1a(2:end-1); %Lo que hago es no perder la energia de la parte negativa agregandoselo a todo lo de la parte positiva excepto al DC y Fs/2(Nyquist)
 fa = fs*(0:(La/2))/La; % vector de frecuencias.
 figure;
 plot(fa,P1a); %Plotea P1
 figure;
 plot(fa,20*log10(P1a)) % grafica con salida en dBs.
 title('Amplitud en el dominio de la frecuencia sin ruido')
 xlabel('f (Hz)')
 ylabel('|P1(f)|')

 figure;

 %Grafico con ruido
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
 title('Amplitud en el dominio de la frecuencia con ruido')
 xlabel('f (Hz)')
 ylabel('|P1(f)|')





