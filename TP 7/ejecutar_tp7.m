function ejecutar_tp7(etapa)
  % Abrir este archivo en el editor de Octave y pulsar Ejecutar (F5).
  % El argumento opcional permite verificar etapas sin abrir el menu.
  base = fileparts(mfilename('fullpath'));
  anterior = pwd(); ruta_anterior = getenv('PATH');
  limpieza = onCleanup(@() restaurar(anterior, ruta_anterior));
  cd(base);
  addpath(fullfile(base,'Octave'));
  herramientas = fullfile(OCTAVE_HOME(),'bin');
  setenv('PATH',[herramientas pathsep ruta_anterior]);
  interactivo = nargin == 0;
  while true
    if interactivo
      etapa = menu('TP7 - Elegi cada paso en orden', ...
        '1. Disenar filtro de 200 Hz', ...
        '2. Compilar los dos programas C', ...
        '3. Ejecutar convolucion temporal', ...
        '4. Ejecutar convolucion por FFT', ...
        '5. Verificar y generar graficas', ...
        '6. Ver graficas en Windows', 'Salir');
    end
    if etapa == 0 || etapa == 7, return; end
    try
      switch etapa
        case 1
          disenar_filtro();
        case 2
          assert(exist('C/filter.h','file') == 2,'Primero realiza el paso 1.');
          if ~exist('C/build','dir'), mkdir('C/build'); end
          gcc = fullfile(herramientas,'gcc.exe');
          assert(exist(gcc,'file') == 2,'No se encontro GCC en la instalacion de Octave.');
          for p = {'timeconv','fastconv_rt'}
            comando = sprintf('"%s" -std=c11 -O2 -Wall -Wextra -Werror C/%s.c -o C/build/%s.exe -lm',gcc,p{1},p{1});
            [estado,salida] = system(comando);
            assert(estado == 0,salida);
          end
        case {3,4}
          if etapa == 3
            programa='timeconv'; metodo='temporal';
          else
            programa='fastconv_rt'; metodo='fft';
          end
          assert(exist(['C/build/' programa '.exe'],'file') == 2,'Primero realiza el paso 2.');
          for senal = {'ruido','senos','impulso'}
            exe = fullfile(base,'C','build',[programa '.exe']);
            destino = fullfile(base,'Resultados',[senal{1} '_' metodo '.csv']);
            comando = sprintf('"%s" %s "%s" 8192',exe,senal{1},destino);
            [estado,salida] = system(comando);
            assert(estado == 0,salida);
          end
        case 5
          verificar_y_graficar();
        case 6
          ver_graficas(base);
          % Volver a Octave sin abrir otro menu o mensaje modal encima.
          fprintf('Carpeta Resultados abierta. Haz doble clic en una imagen PNG.\n');
          return;
        otherwise
          error('Etapa no valida.');
      end
      if interactivo
        uiwait(msgbox(sprintf('Paso %d terminado. Podes continuar con el siguiente.',etapa),'TP7'));
      else
        fprintf('Paso %d terminado.\n',etapa);
      end
    catch err
      if interactivo
        uiwait(errordlg(err.message,'No se pudo completar el paso'));
      else
        rethrow(err);
      end
    end
    if ~interactivo, return; end
  end
end

function restaurar(carpeta,ruta)
  cd(carpeta);
  setenv('PATH',ruta);
end

