clc;
clear;

vf = logspace(1, 5, 1000); %vector de frecuencias
W = 2 * pi * vf; %frecuencia angular
s = 1j * W;


k1 = 0.1592; %cambian los coeficientes
k2 = 0.586;
C = 10e-9;

R1 = 1000;
R = k1/(C*1000);
Rf = R1*k2;
K = 1+ k1
Wn = 1/(R*C);
Q = 1/(3-K);

%funcion de transferencia
H = K./((1-K)*R*s*C+1+2*(R*s*C).^2);
Hdb = 20*log10(abs(H));

%plot log

semilogx(vf,Hdb,"b");
grid on;
xlabel("frecuencia [HZ]");
ylabel("Amplitud [db]");
title("Ejercicio 5");








