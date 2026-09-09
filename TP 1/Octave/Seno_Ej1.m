%Ejercicio con resolución

% Genero parametros de simulacion
  % Genero parametros de simulacion

  fs = 2000;          % Frecuencia de muestreo
  t_dur = 0.2;         % Duración de la simulación en segundos
  N1 = fs*t_dur;       % Cantidad de muestras
  n1 = 0:N1-1;         % Ancho de simulacion
  t1 = n1/fs;          % Cada cuanto muestrea

  % Defino Resolucion
  bits = 3;  % bits de cuantización
  levels = 2^bits;  % niveles de cuantización

  % Parametros para el seno
  A_in=1;
  F_in=50;

 x = Seno_function(A_in,F_in,t1);

 %Aplico la resolucion para cuantizar

  x_quantized = round((x + 1) * (levels/2 - 1)) / (levels/2 - 1) - 1;

  % Para cuantizar lo que hace es subirlo de 0 a 2 (Esto porque la amplitud actual es de [-1;1]
  % Despues lo escala porque el redondeo es a numeros enteros.
  % Redondeo y luego lo desescalo -> Esto porque necesito trabajarlo en numeros con coma
  % Elimino el offset.

  plot(t1, x, 'b', 'LineWidth', 1);
  hold on;
  plot(t1, x_quantized, 'r', 'LineWidth', 1.5);
  xlabel('Tiempo [s]');
  ylabel('Amplitud');
  title(['Señal Cuantizada a ', num2str(bits), ' bits']);
  legend('Original', 'Cuantizada');
  grid on;

  %Ejercicio Finalizado.
