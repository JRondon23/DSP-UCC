function ver_graficas(base)
  % El visor de Octave puede ubicar las ventanas fuera de pantalla con escalado DPI.
  % Usar el Explorador y el visor de imagenes de Windows para los PNG exportados.
  if nargin == 0, base=fileparts(fileparts(mfilename('fullpath'))); end
  archivos={'respuesta_filtro','ruido_tiempo','ruido_frecuencia','senos_tiempo','senos_frecuencia'};
  for k=1:numel(archivos)
    assert(exist(fullfile(base,'Resultados',[archivos{k} '.png']),'file')==2, ...
           'Primero genera las graficas con los pasos 1 y 5.');
  end
  % Cerrar solo los visores de TP7, nunca las otras figuras del usuario.
  for nombre=[archivos,{'TP7 - Graficas'}]
    delete(findall(0,'type','figure','name',nombre{1}));
  end
  carpeta = strrep(fullfile(base,'Resultados'),'/','\');
  % explorer.exe es parte de Windows; no requiere winopen ni PowerShell.
  pid = system(['explorer.exe "' carpeta '"'],false,'async');
  assert(pid > 0,'No se pudo abrir el Explorador de Windows.');
end
