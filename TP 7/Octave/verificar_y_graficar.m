function verificar_y_graficar()
  graphics_toolkit('gnuplot');
  base=fileparts(fileparts(mfilename('fullpath')));
  result=fullfile(base,'Resultados');
  h=dlmread(fullfile(result,'coeficientes.csv'));
  fid=fopen(fullfile(result,'verificacion.csv'),'w'); assert(fid>=0);
  fprintf(fid,'senal,muestras,error_temporal,error_fft,error_entre_metodos\n');
  for nombre={'ruido','senos','impulso'}
    s=nombre{1};
    a=dlmread(fullfile(result,[s '_temporal.csv']),',',1,0);
    b=dlmread(fullfile(result,[s '_fft.csv']),',',1,0);
    ref=conv(a(1:8192,3),h);
    e1=max(abs(a(:,4)-ref)); e2=max(abs(b(:,4)-ref));
    e3=max(abs(a(:,4)-b(:,4)));
    assert(rows(a)==8192+length(h)-1);
    assert(isequal(a(:,3),b(:,3)));
    assert(max([e1 e2 e3])<1e-10);
    fprintf(fid,'%s,%d,%.12g,%.12g,%.12g\n',s,rows(a),e1,e2,e3);
    if strcmp(s,'impulso'), continue; end
    figure('visible','off'); graphics_toolkit(gcf,'gnuplot');
    subplot(2,1,1); curvas=plot(a(1:800,2),a(1:800,3),'r-');
    title([s ': entrada']); xlabel('Tiempo [s]'); ylabel('Amplitud');
    formato_grafica(gca,curvas,{'Entrada'});
    subplot(2,1,2); curvas=plot(a(1:800,2),a(1:800,4),'r-',b(1:800,2),b(1:800,4),'r--');
    title('Salida filtrada'); xlabel('Tiempo [s]'); ylabel('Amplitud');
    formato_grafica(gca,curvas,{'Temporal','FFT'});
    print(fullfile(result,[s '_tiempo.png']),'-dpng','-S1200,1000'); close;
    % Ventana comun en regimen permanente. Escala unilateral en amplitud.
    idx=1025:8192; L=length(idx); w=0.5-0.5*cos(2*pi*(0:L-1)'/(L-1));
    X=fft(a(idx,3).*w); Y=fft(a(idx,4).*w);
    bins=1:(floor(L/2)+1); f=(bins-1)*8000/L;
    escala=2*ones(length(bins),1)/sum(w); escala(1)=1/sum(w); escala(end)=1/sum(w);
    figure('visible','off'); graphics_toolkit(gcf,'gnuplot');
    curvas=plot(f,20*log10(max(abs(X(bins)).*escala,1e-12)),'r-',f,20*log10(max(abs(Y(bins)).*escala,1e-12)),'r--');
    grid on; xlabel('Frecuencia [Hz]'); ylabel('Amplitud [dB ref. 1]');
    title([s ': espectro de entrada y salida']); ylim([-120 10]);
    formato_grafica(gca,curvas,{'Entrada','Salida'});
    print(fullfile(result,[s '_frecuencia.png']),'-dpng','-S1200,1000'); close;
  end
  fclose(fid);
  % Bloques incompletos y fronteras: la referencia incluye toda la cola.
  for count=[1 127 128 129 257]
    for metodo={'timeconv','fastconv_rt'}
      exe=fullfile(base,'C','build',[metodo{1} '.exe']);
      archivo=fullfile(result,'prueba_corta.csv');
      [status,msg]=system(sprintf('"%s" ruido "%s" %d',exe,archivo,count));
      assert(status==0,msg);
      a=dlmread(archivo,',',1,0); ref=conv(a(1:count,3),h);
      assert(rows(a)==count+length(h)-1);
      assert(max(abs(a(:,4)-ref))<1e-10);
    end
  end
  delete(fullfile(result,'prueba_corta.csv'));
  fprintf('Verificacion OK: ruido, senos, impulso y bloques incompletos.\n');
end




