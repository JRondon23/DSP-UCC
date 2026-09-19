clc;
clear;

%vector de posicion
n_x = 0:4;
n_h = 0:4;

%definicion x(n), h(n)
%x = [1,2,4,0]; %punto a
%h = [1,1,1,1]; %punto a

%x = [1,2,-1]; %punto b
%h = [1,2,-1]; %punto b

%x = [1,2,3,4,5]; %punto c
%h = [1,0,0,0,0]; %punto c

x = [1,1,0,1,1]; %punto d
h = [1,-2,-3,4,0]; %punto d

%convolucion
y = conv(x,h);

%calculo de los ejes de tiempo
n_y = (min(n_x) + min(n_h)) : (max(n_x) + max(n_h));

% Ventana unica para todos los graficos
figure;

% 1. Grafico superior: x[n]
subplot(3, 1, 1);
stem(n_x, x, 'filled', 'linewidth', 1.5,'r');
grid on; xticks(n_x);
xlabel('n'); ylabel('x[n]');
title('Senal x[n]');

% 2. Grafico del medio: h[n]
subplot(3, 1, 2);
stem(n_h, h, 'filled', 'linewidth', 1.5,'g');
grid on; xticks(n_h);
xlabel('n'); ylabel('h[n]');
title('Respuesta al impulso h[n]');

% 3. Grafico inferior: y[n]
subplot(3, 1, 3);
stem(n_y, y, 'filled', 'linewidth', 1.5);
grid on; xticks(n_y);
xlabel('n'); ylabel('y[n]');
title('Convolucion y[n] = x[n] * h[n]');

