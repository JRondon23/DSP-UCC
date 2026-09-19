clc;
clear;

% 1. Definir la senal y su eje de tiempo
n = 0:4;
x = [1, 2, 4, 2, 1];

% 2. Calcular la autocorrelacion y sus desfases (lags)
[r_xx, lags] = xcorr(x);

% 3. Graficar la autocorrelacion
figure;
stem(lags, r_xx, 'filled', 'linewidth', 1.5);
grid on; xticks(lags);
xlabel('Desfase (k)');
ylabel('R_{xx}[k]');
title('Autocorrelacion de x[n]');
