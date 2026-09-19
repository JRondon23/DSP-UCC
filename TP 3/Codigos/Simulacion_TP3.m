% genera la se#al sinusoidal
signal_freq=100;
signal_amp=100;
sampling_freq=8000;
sample_number = 2000;
for i=1:sample_number
 signal(i) = round(signal_amp*sin(2*pi*(i-1)*signal_freq/sampling_freq));
end
plot(signal);
plot(signal, 'o-b');
figure

sampling_freq2=500;

for i=1:sample_number
 signal2(i) = exp((i-1)*1/sampling_freq2);
end
plot(signal2);
plot(signal2, 'o-b');
figure


convolucion = conv(signal,signal2);
plot(convolucion);
plot(convolucion, 'o-b');
ylim([0,7000])
xlim([0,2000]);
figure


% genera el archivo de cabecera con las muestras de la se#al
array = signal;
min_index = 1;
max_index = length(array);
file_name = 'test_signal.h';
const_name = 'ELEMENTS';
array_name = ['short test_signal[' const_name ']={'];
% crea el archivo de cabecera
fid=fopen(file_name,'w');
fprintf(fid,'#define ');
fprintf(fid, const_name);
fprintf(fid, ' %d\n\n', max_index);
fprintf(fid, array_name);
fprintf(fid,'%d, ' , array(min_index:(max_index-1)));
fprintf(fid,'%d' , array(max_index));
fprintf(fid,'};\n');
fclose(fid);


% lee el archivo de texto creado desde el programa en C
file_name = 'archivo1.txt';
[a, in, out] = textread (file_name, "%f %f %f", "headerlines", 1);
subplot(2, 1, 1);
plot(in, 'o-r');
xlim([0,2000]);
title('Entrada');
hold on
subplot(2, 1, 2);
plot(out, 'o-b');
xlim([0,2000]);
title('Salida');
