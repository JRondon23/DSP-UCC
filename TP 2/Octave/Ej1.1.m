clc; % limpia la pantalla
clear; % borra el espacio de trabajo

N =200;
n =0:N-1;

%signals
signal_a = cos(0.01*pi*n);
signal_b = cos(pi*(30/105)*n);
signal_c = cos(3*pi*n);
signal_d = sin(3*n);
signal_e = sin(pi*(62/10)*n);
%plot

figure;
stem(n,signal_a,'r');
title('singal a');
xlabel('n');
ylabel('Amplitud');
grid on;

figure;
stem(n,signal_b,'b');
title('signal b');
xlabel('n');
ylabel('Amplitud');
grid on;


figure;
stem(n,signal_c,'g');
title('signal c');
xlabel('n');
ylabel('Amplitud');
grid on;


figure;
stem(n,signal_d,'y');
title('signal d');
xlabel('n');
ylabel('Amplitud');
grid on;


figure;
stem(n,signal_e,'m');
title('signal e');
xlabel('n');
ylabel('Amplitud');
grid on;

