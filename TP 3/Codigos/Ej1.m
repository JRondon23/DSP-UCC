clc;
clear;


n = -10:10;

%definicion de x
%tramo1 = 1 .*(n >= -1 & n <= 2);
%tramo2 = (1/2) .*(n >=3 & n <= 4);
%x = tramo1 + tramo2;

%Definicion nueva de x
x = @(n) 1 .* (n >= -1 & n <= 2) + (1/2) .* (n >= 3 & n <= 4);

%definicion de u
u = @(n) n>= 0;
%U shifteada
u_shift = u(n-2)

%Delta de dirac
delta_d = n == 0
%delta de dirac
delta_shift = (n ==3);

% Evaluar x[n-1]
% Para evaluar x en (n-1), definimos los tramos usando (n-1)
tramo2_shift = 1 .* ((n-1) >= -1 & (n-1) <= 2);
tramo3_shift = (1/2) .* ((n-1) >= 3 & (n-1) <= 4);
x_shift = tramo2_shift + tramo3_shift;

y = x(n.^2);


stem(n,y,'filled','linewidth', 1.5);
grid on;
xticks(n);

xlabel('Muestras (n)');
ylabel('Amplitud y[n]');
title('Senial por partes y[n]');






