function disenar_filtro()
  pkg load signal;
  graphics_toolkit('gnuplot');
  base = fileparts(fileparts(mfilename('fullpath')));
  fs = 8000; fc = 200; fp = 100; fr = 350;
  % Sinc ideal truncada con ventana Kaiser: corte nominal de -6 dB.
  n = -64:64;
  h = (2*fc/fs)*sinc((2*fc/fs)*n).*kaiser(129,5.65)';
  h = h/sum(h);
  [H,f] = freqz(h,1,65536,fs);
  rechazo = -20*log10(max(abs(H(f>=fr))));
  paso = 20*log10(abs(H(f<=fp)));
  corte_dB = 20*log10(abs(sum(h .* exp(-1i*2*pi*fc/fs*(0:length(h)-1)))));
  assert(abs(corte_dB + 6.0206) < 0.1);
  assert(rechazo >= 40);
  assert(max(abs(paso)) < 1);
  dlmwrite(fullfile(base,'Resultados','coeficientes.csv'),h(:),'precision','%.17g');
  fid = fopen(fullfile(base,'C','filter.h'),'w'); assert(fid>=0);
  fprintf(fid,'/* Generado por Octave/disenar_filtro.m; fs=8000 Hz */\n#ifndef TP7_FILTER_H\n#define TP7_FILTER_H\n#define N 129\nstatic const double h[N] = {\n');
  fprintf(fid,'%.17g,\n',h); fprintf(fid,'};\n#endif\n'); fclose(fid);
  % Exportar tambien a float para el FIR temporal de la placa.
  destino = fullfile(base,'MXCube','TP7','Core','Inc');
  if exist(destino,'dir')
    fid=fopen(fullfile(destino,'tp7_fir_coeffs.h'),'w'); assert(fid>=0);
    fprintf(fid,'/* Generado por Octave/disenar_filtro.m. No editar a mano. */\n#ifndef TP7_FIR_COEFFS_H\n#define TP7_FIR_COEFFS_H\n#define TP7_FIR_TAPS 129U\n#define TP7_FIR_FS_HZ 8000U\n#define TP7_FIR_CUTOFF_HZ 200U\nstatic const float fir_h[TP7_FIR_TAPS] = {\n');
    fprintf(fid,'%.9ef,\n',single(h)); fprintf(fid,'};\n#endif\n'); fclose(fid);
  end
  fid = fopen(fullfile(base,'Resultados','diseno.csv'),'w');
  fprintf(fid,'fs_Hz,taps,retardo_muestras,corte_Hz,ganancia_corte_dB,borde_paso_verificado_Hz,borde_rechazo_Hz,perdida_max_paso_dB,atenuacion_rechazo_dB\n');
  fprintf(fid,'%d,%d,%d,%d,%.9g,%d,%d,%.9g,%.9g\n',fs,length(h),64,fc,corte_dB,fp,fr,-min(paso),rechazo); fclose(fid);
  set(0,'defaultfigurevisible','off');
  figure; graphics_toolkit(gcf,'gnuplot'); curva=plot(f,20*log10(max(abs(H),1e-8)),'r-');
  xlabel('Frecuencia [Hz]'); ylabel('Ganancia [dB]'); ylim([-100 5]);
  title('FIR pasabajos: fc = 200 Hz, 129 coeficientes, fs = 8000 Hz');
  formato_grafica(gca,curva,{'Filtro FIR'});
  print(fullfile(base,'Resultados','respuesta_filtro.png'),'-dpng','-S1200,900'); close;
  fprintf('Corte nominal: %d Hz, ganancia: %.5f dB\n',fc,corte_dB);
  fprintf('FIR: rechazo %.3f dB; perdida maxima en paso %.5f dB\n',rechazo,-min(paso));
end




