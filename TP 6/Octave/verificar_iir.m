function verificar_iir(graficar = true)
  pkg load signal;
  carpeta = fileparts(mfilename('fullpath'));
  pruebas = fullfile(carpeta, '..', 'Tests');
  ejecutable = fullfile(pruebas, 'iir_sim.exe');
  assert(exist(ejecutable, 'file') == 2, 'Compilar Tests/iir_sim.c primero (ver README_IIR.md)');
  resultados = fullfile(pruebas, 'resultados');
  if !exist(resultados, 'dir'), mkdir(resultados); endif
  fs = 8000; n = (0:79999)';
  rng(6);
  senales = {[1; zeros(length(n)-1,1)], ones(size(n)), ...
             (sin(2*pi*100*n/fs)+sin(2*pi*250*n/fs)+sin(2*pi*400*n/fs)+sin(2*pi*1000*n/fs))/4, ...
             2*rand(size(n))-1};
  etiquetas = {'impulso', 'escalon', 'multitono', 'ruido'};
  nombres = {'pasabanda', 'rechazabanda'};
  for k = 1:2
    datos = load(fullfile(carpeta, [nombres{k}, '.mat']));
    sos = datos.sos;
    for j = 1:length(senales)
      x = double(single(senales{j}));
      entrada = fullfile(resultados, 'entrada.txt');
      salida = fullfile(resultados, 'salida.txt');
      dlmwrite(entrada, x, 'precision', '%.9g');
      cmd = sprintf('"%s" %d "%s" "%s"', ejecutable, k-1, entrada, salida);
      [status, mensaje] = system(cmd);
      assert(status == 0, mensaje);
      yc = load(salida);
      y = x;
      for s = 1:rows(sos)
        y = filter(sos(s,1:3), sos(s,4:6), y);
      endfor
      assert(length(yc) == length(y));
      err = max(abs(y-yc));
      assert(all(isfinite(yc)) && err < 0.001, 'C difiere de Octave');
      fprintf('%s / %s: error maximo C vs Octave = %.8g\n', nombres{k}, etiquetas{j}, err);
      if j == 3
        % Ganancia medida en la salida real C, descartando un segundo inicial.
        m = (fs:length(n)-1)';
        for f = [100 250 400 1000]
          amp_in = 2*abs(sum(x(m+1).*exp(-2i*pi*f*m/fs)))/length(m);
          amp_out = 2*abs(sum(yc(m+1).*exp(-2i*pi*f*m/fs)))/length(m);
          fprintf('  %4d Hz: %8.3f dB\n', f, 20*log10(amp_out/amp_in));
        endfor
        if graficar
          figure(k+2); clf;
          subplot(2,1,1); plot(n(1:800)/fs, [x(1:800), yc(1:800)]); grid on;
          title([nombres{k}, ': entrada y salida C']); xlabel('s'); ylabel('Amplitud');
          legend('Entrada', 'C');
          % Registro estacionario de un segundo, sin transitorio inicial.
          tramo = fs+1:2*fs;
          espectro = 2*abs(fft([x(tramo), yc(tramo)]))/fs;
          subplot(2,1,2); plot(0:fs/2, 20*log10(max(espectro(1:fs/2+1,:),1e-10)));
          grid on; ylim([-100 5]); xlabel('Hz'); ylabel('Amplitud (dB)');
          legend('Entrada', 'C');
        endif
      endif
    endfor
  endfor
  disp('Verificacion C/Octave completada: ambos filtros, cuatro entradas de 10 s.');
endfunction
