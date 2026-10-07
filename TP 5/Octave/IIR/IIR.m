%filtro IIR pasa bajo (butterworth)
pkg load signal
clear all; close all; clc;


fs = 8000;
fc = 1100; % Frecuencia para cuando hay una sola Fc
%fc1 = 300;
%fc2 = 500;
order = 6;
fmax = fs/2;
[b, a] = cheby1(order,1,fc/fmax, 'high');
freqz (b, a);

% --- Generar archivo de cabecera con los coeficientes (filter.h) ---
file_name = 'filter_IIR.h';
const_name = 'N_iir'
fid = fopen(file_name, 'w');

fprintf(fid, '#define %s %d\n\n', const_name, length(a));
fprintf(fid, 'float a[%s] = {\n    ', const_name);
fprintf(fid, '%f, ', a(1:end-1));
fprintf(fid, '%f\n', a(end));
fprintf(fid, '};\n');

fprintf(fid, 'float b[%s] = {\n    ', const_name);
fprintf(fid, '%f, ', b(1:end-1));
fprintf(fid, '%f\n', b(end));
fprintf(fid, '};\n');
fclose(fid);

% Generar Seniales de entrada
sample_number = 4000;
n = 0:(sample_number-1); % Vector de índices de tiempo

% 1. Señal sinusoidal simple (1000 Hz)
signal_1kHz = round(2 * sin(2*pi*n*1000/fs));

% 2. Señal compuesta por múltiples frecuencias
freq_range = [10, 20, 40, 80, 100, 200, 400, 800, 1000, 1200, 1400, 1800, 2000, 2100, 2200, 2400, 2800, 3000, 3400];
comp_signal = zeros(1, sample_number);

for j = 1:length(freq_range)
    comp_signal = comp_signal + round(2 * sin(2*pi*n*freq_range(j)/fs));
end

figure(2);
plot(comp_signal, 'o-b');
title('Señal de entrada Compuesta (Tiempo)');
xlabel('Muestra'); ylabel('Amplitud');


% Se nial aleatoria (Ruido)
random_signal = round(2 * (1 - 2*rand(1, sample_number)));
figure(6);
plot(random_signal, 'o-b');
title('Señal de entrada random (Tiempo)');
xlabel('Muestra'); ylabel('Amplitud');



% --- Generar archivo de cabecera de la señal de entrada (signal.h) ---
senal_trabajo = comp_signal;

file_name = 'signal_IIR.h';
const_name = 'LINE_ELEMENTS';
fid = fopen(file_name, 'w');

fprintf(fid, '#define %s %d\n\n', const_name, length(senal_trabajo));
fprintf(fid, 'short signal[%s] = {\n    ', const_name);
fprintf(fid, '%d, ', senal_trabajo(1:end-1));
fprintf(fid, '%d\n', senal_trabajo(end));
fprintf(fid, '};\n');
fclose(fid);



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  ANÁLISIS FFT DIRECTO (SEÑAL ORIGINAL)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
disp('Calculando FFT de la señal original...');

X_in = senal_trabajo(:)'; % Forzamos vector fila
L_in = length(X_in);
Y_in = fft(X_in);

P2_in = abs(Y_in/L_in);
P1_in = P2_in(1:floor(L_in/2)+1);
P1_in(2:end-1) = 2*P1_in(2:end-1);
f_in = fs*(0:floor(L_in/2))/L_in;

figure(3); % Nueva ventana para FFT original
subplot(2,1,1);
plot(f_in, P1_in, 'LineWidth', 1.5, 'color', 'b');
title('FFT Señal Original (Lineal)');
xlabel('f (Hz)'); ylabel('Amplitud');
set(gca, 'xtick', 0:200:fs/2);
grid on;

P1_in(P1_in < 1e-7) = 1e-7; % Evitar log(0)
subplot(2,1,2);
plot(f_in, 20*log10(P1_in), 'LineWidth', 1.5, 'color', 'b');
title('FFT Señal Original (dB)');
xlabel('f (Hz)'); ylabel('Amplitud (dB)');
set(gca, 'xtick', 0:200:fs/2);
grid on;
drawnow;


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  CARGA DE ARCHIVO DE SALIDA Y FFT (SEÑAL FILTRADA)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
file_name = 'iir_out.txt';

if exist(file_name, 'file')
    disp('Leyendo archivo de salida y calculando FFT del filtro...');
    [a, iir_out] = textread(file_name, "%f %f", "headerlines", 1);

    figure(4);
    plot(iir_out, 'color', 'r');
    title('Salida del Filtro (IIR OUT en Tiempo)');
    xlabel('Muestra'); ylabel('Amplitud');
    grid on;
    drawnow;

    % --- FFT DE LA SALIDA ---
    X_out = iir_out(:)'; % Forzamos vector fila
    L_out = length(X_out);
    Y_out = fft(X_out);

    P2_out = abs(Y_out/L_out);
    P1_out = P2_out(1:floor(L_out/2)+1);
    P1_out(2:end-1) = 2*P1_out(2:end-1);
    f_out = fs*(0:floor(L_out/2))/L_out;

    figure(5); % Nueva ventana para FFT filtrada
    subplot(2,1,1);
    plot(f_out, P1_out, 'LineWidth', 1.5, 'color', 'r');
    title('FFT Salida Filtrada (Lineal)');
    xlabel('f (Hz)'); ylabel('Amplitud');
    set(gca, 'xtick', 0:200:fs/2);
    grid on;

    P1_out(P1_out < 1e-7) = 1e-7; % Evitar log(0)
    subplot(2,1,2);
    plot(f_out, 20*log10(P1_out), 'LineWidth', 1.5, 'color', 'r');
    title('FFT Salida Filtrada (dB)');
    xlabel('f (Hz)'); ylabel('Amplitud (dB)');
    set(gca, 'xtick', 0:200:fs/2);
    grid on;
    drawnow;
else
    disp('Nota: iir_out.txt no encontrado. Ejecuta tu firmware C primero para generarlo.');
end

disp('Script finalizado. Presiona ENTER en la consola para cerrar todo...');
pause;


