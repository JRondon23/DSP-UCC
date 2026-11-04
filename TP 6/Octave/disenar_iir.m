function disenar_iir(graficar = true)
  % Ejecutar: disenar_iir() o disenar_iir(false) sin ventanas.
  % Genera ambos IIR directamente en Core/Inc, independientemente del cwd.
  pkg load signal;
  carpeta = fileparts(mfilename('fullpath'));
  destino = fullfile(carpeta, '..', 'STM32', 'TP5-DSP', 'Core', 'Inc', 'iir_coeffs.h');
  fs = 8000; rp = 1; rs = 40;
  % Comprobar el muestreo configurado ANTES de sobrescribir coeficientes.
  % Un ARR=999 en lugar de 9999 solicita 80 kHz y desplaza toda la respuesta.
  proyecto = fullfile(carpeta, '..', 'STM32', 'TP5-DSP');
  ioc = fileread(fullfile(proyecto, 'TP5-DSP.ioc'));
  timer = fileread(fullfile(proyecto, 'Core', 'Src', 'tim.c'));
  reloj = regexp(ioc, 'RCC\.APB1TimFreq_Value=([0-9]+)', 'tokens', 'once');
  psc_ioc = regexp(ioc, 'TIM2\.Prescaler=([0-9]+)', 'tokens', 'once');
  arr_ioc = regexp(ioc, 'TIM2\.PeriodNoDither=([0-9]+)', 'tokens', 'once');
  psc_c = regexp(timer, 'htim2\.Init\.Prescaler\s*=\s*([0-9]+)', 'tokens', 'once');
  arr_c = regexp(timer, 'htim2\.Init\.Period\s*=\s*([0-9]+)', 'tokens', 'once');
  assert(!isempty(reloj) && !isempty(psc_ioc) && !isempty(arr_ioc) && ...
         !isempty(psc_c) && !isempty(arr_c), 'No se pudo verificar TIM2');
  assert(strcmp(psc_ioc{1}, psc_c{1}) && strcmp(arr_ioc{1}, arr_c{1}), ...
         'TIM2 difiere entre CubeMX (.ioc) y tim.c: regenerar y comprobar');
  fs_timer = str2double(reloj{1}) / ...
             ((str2double(psc_c{1})+1)*(str2double(arr_c{1})+1));
  assert(abs(fs_timer-fs) < 0.01, ...
         sprintf('TIM2 solicita %.0f Hz y el diseno usa %.0f Hz. Corregir TIM2 antes de exportar.', fs_timer, fs));
  fprintf('Muestreo verificado en .ioc y tim.c: %.0f Hz\n', fs_timer);
  nombres = {'pasabanda', 'rechazabanda'};
  wp = {[200 300], [200 700]};
  ws = {[150 350], [300 500]};
  fid = fopen(destino, 'w');
  assert(fid >= 0, 'No se pudo abrir iir_coeffs.h');
  unwind_protect
    fprintf(fid, '/* Generado por TP 6/Octave/disenar_iir.m. No editar a mano. */\n');
    fprintf(fid, '#ifndef IIR_COEFFS_H\n#define IIR_COEFFS_H\n#define IIR_FS_HZ %d\n', fs);
    for k = 1:2
      [orden, wn] = cheb2ord(wp{k}/(fs/2), ws{k}/(fs/2), rp, rs);
      if k == 1
        [z, p, ganancia] = cheby2(orden, rs, wn);
      else
        [z, p, ganancia] = cheby2(orden, rs, wn, 'stop');
      endif
      % Evitar pasar por polinomios de orden alto (b,a) antes de zp2sos.
      sos = real(zp2sos(z, p, ganancia));
      for s = 1:rows(sos)
        sos(s,:) = sos(s,:) / sos(s,4);
      endfor
      % Validar los coeficientes tal como los recibe el float de la placa.
      sos = double(single(sos));
      for s = 1:rows(sos)
        assert(all(abs(roots(sos(s,4:6))) < 1), 'Seccion inestable en float');
      endfor
      f = unique([linspace(0, fs/2, 32769), wp{k}, ws{k}]);
      h = ones(size(f));
      for s = 1:rows(sos)
        h = h .* freqz(sos(s,1:3), sos(s,4:6), 2*pi*f/fs);
      endfor
      db = 20*log10(max(abs(h), 1e-15));
      if k == 1
        paso = f >= wp{k}(1) & f <= wp{k}(2);
        rechazo = f <= ws{k}(1) | f >= ws{k}(2);
      else
        paso = f <= wp{k}(1) | f >= wp{k}(2);
        rechazo = f >= ws{k}(1) & f <= ws{k}(2);
      endif
      assert(min(db(paso)) >= -rp-0.02);
      assert(max(db(rechazo)) <= -rs+0.02);
      if k == 1
        % Mostrar los bordes de paso por separado de los cortes de -3 dB.
        dentro_3db = find(db >= -3);
        fprintf('Pasabanda: bordes de paso 200 y 300 Hz (-1 dB); cortes -3 dB aprox. %.2f y %.2f Hz\n', ...
                f(dentro_3db(1)), f(dentro_3db(end)));
      endif
      fprintf('%s: orden total %d, %d SOS; paso %.3f dB; rechazo %.3f dB\n', ...
              nombres{k}, length(p), rows(sos), min(db(paso)), max(db(rechazo)));
      fprintf(fid, '#define IIR_%s_SECTIONS %d\n', upper(nombres{k}), rows(sos));
      fprintf(fid, 'static const float iir_%s[%d][6] = {\n', nombres{k}, rows(sos));
      for s = 1:rows(sos)
        fprintf(fid, '    {');
        % 9 cifras significativas bastan para conservar float32.
        fprintf(fid, '%.9ef, ', sos(s,1:5));
        fprintf(fid, '%.9ef},\n', sos(s,6));
      endfor
      fprintf(fid, '};\n');
      if graficar
        figure(k); clf;
        subplot(2,1,1); plot(f, db); grid on; ylim([-100 5]);
        title(['IIR Chebyshev II ', nombres{k}]); xlabel('Hz'); ylabel('Ganancia (dB)');
        subplot(2,1,2); plot(f, unwrap(angle(h))); grid on;
        xlabel('Hz'); ylabel('Fase (rad)');
      endif
      save(fullfile(carpeta, [nombres{k}, '.mat']), 'sos', 'fs', 'wp', 'ws', 'rp', 'rs');
      % CSV compacto para comparar con las mediciones del laboratorio.
      indices = unique([1:16:length(f), length(f), find(ismember(f, [wp{k}, ws{k}]))]);
      dlmwrite(fullfile(carpeta, [nombres{k}, '_respuesta.csv']), [f(indices)', db(indices)'], ...
               'precision', '%.12g');
    endfor
    fprintf(fid, '#endif\n');
  unwind_protect_cleanup
    fclose(fid);
  end_unwind_protect
  fprintf('Coeficientes exportados a: %s\n', destino);
endfunction
