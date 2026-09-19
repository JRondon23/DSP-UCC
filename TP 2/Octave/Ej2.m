clc; % limpia la pantalla
clear; % borra el espacio de trabajo

%fs = 300
fs_1 = 300; % freqcuencia de sampleo
t_dur = 0.1; % duracion de sim
N1 = fs_1*t_dur; % cantidad de muestras
n1 =0:N1-1; % muestras
fs_2 = 1000;
N2 = fs_2*t_dur;
n2=0:N2-1;




%x(t) = 3sin(2*pi*50*t)
%x(n) = 3sin(2*pi*50/fs*n)
%f0_1= 50/300
%T0_1 = 6
%f0_2= 50/1000
%T0_2 = 20

x_1 = 3*sin(2*pi*(50/fs_1)*n1);


x_2 = 3*sin(2*pi*(50/fs_2)*n2);

figure;
stem(n1,x_1,'r');
title('Frecuencia 300');
xlabel('Muestra n');
ylabel('Amplitud');
grid on;

figure;
stem(n2,x_2,'b');
title('Frecuencia 1000');
xlabel('Muestra n');
ylabel('Amplitud');
grid on;



