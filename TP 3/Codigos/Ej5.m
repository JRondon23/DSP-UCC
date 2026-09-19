clc;
clear;


%vector de tiempo
n_x = 0:3;
n_h = 0:3;


%definicion x y h
%x = [1,2,4,0,0]; %punto a
%h = [1,1,1,1,1]; %punto a

%x = [0,1,-2,3,-4]; %punto b
%h = [(1/2),1,2,1,(1/2)]; %punto b

%x = [1,2,3,4]; %punto c
%h = [4,3,2,1]; %punto c

x = [1,2,3,4]; %punto d
h = [1,2,3,4]; %punto d

%convolucion
y = conv(x,h);

%calculo de los ejes de tiempo
n_y = (min(n_x) + min(n_h)) : (max(n_x) + max(n_h));

%calculo de la correlacion
[r_xh,lags] = xcorr(x,h);


% Ventana unica para todos los graficos
figure;

% Grafico: correlacion
subplot(2, 1, 1);
stem(lags, r_xh, 'filled', 'linewidth', 1.5,'r');
grid on; xticks(lags);
xlabel('Desfase (k)');
ylabel('r_{xh}[k]');
title('Correlacion Cruzada r_{xh}[k]');

%Grafico inferior: y[n] convolucion
subplot(2, 1, 2);
stem(n_y, y, 'filled', 'linewidth', 1.5);
grid on; xticks(n_y);
xlabel('n'); ylabel('y[n]');
title('Convolucion y[n] = x[n] * h[n]');

%grafico de senales
figure

% 1. Grafico superior: x[n]
subplot(2, 1, 1);
stem(n_x, x, 'filled', 'linewidth', 1.5,'r');
grid on; xticks(n_x);
xlabel('n'); ylabel('x[n]');
title('Senal x[n]');

% 2. Grafico del medio: h[n]
subplot(2, 1, 2);
stem(n_h, h, 'filled', 'linewidth', 1.5,'g');
grid on; xticks(n_h);
xlabel('n'); ylabel('h[n]');
title('Respuesta al impulso h[n]');




