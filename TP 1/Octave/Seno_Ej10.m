% 10) Repita el ejercicio anterior utilizando diferentes cantidades de muestras: 50,
% 100, 500 y 1000. Analice cómo afecta la cantidad de muestras a la resolución en
% frecuencia de la FFT.

% Parametros para el seno
A_in = 1;
F_in = 1*10^3;

% Genero parametros de simulacion
fs = 10000;                    % Frecuencia de muestreo
N1_vals = [50, 100, 500, 1000]; % Cantidad de muestras

for i = 1:length(N1_vals)
  N1 = N1_vals(i);
  n1 = 0:N1-1;
  t1 = n1/fs;

  x = Seno_function(A_in, F_in, t1);

  L = length(x);
  y = fft(x);
  P2 = abs(y/L);
  P1 = P2(1:floor(L/2)+1);
  P1(2:end-1) = 2*P1(2:end-1);
  f = fs*(0:floor(L/2))/L;

  figure;
  plot(f, P1);
  title(['Amplitud en frecuencia - N = ' num2str(N1)])
  xlabel('f (Hz)')
  ylabel('|P1(f)|')
  grid on

  figure;
  plot(f, 20*log10(P1))
  title(['Amplitud en dB - N = ' num2str(N1)])
  xlabel('f (Hz)')
  ylabel('|P1(f)| (dB)')
  grid on
end
