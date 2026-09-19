clc; % limpia la pantalla
clear; % borra el espacio de trabajo

%simulacion

vf = 0:0.1:2000;
R = 1000;
C = 159.19e-9;
w = 2*pi*vf;
H =1./(1+(1j*w*R*C));

%log

dbH = 20*log10(abs(H));

%plot

semilogx(vf,dbH,"r");
grid on;
xlabel("frecuencia [HZ]");
ylabel("decibelios [db]");
title("respuesta filtro RC");





