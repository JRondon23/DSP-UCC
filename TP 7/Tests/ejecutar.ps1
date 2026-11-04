$ErrorActionPreference = 'Stop'
$base = Split-Path $PSScriptRoot -Parent
$octaveBin = Get-ChildItem -LiteralPath "$env:ProgramFiles/GNU Octave" -Directory |
    Sort-Object Name -Descending | ForEach-Object { Join-Path $_.FullName 'mingw64/bin' } |
    Where-Object { Test-Path (Join-Path $_ 'octave-cli.exe') } | Select-Object -First 1
if (-not $octaveBin) { throw 'Instalar GNU Octave con el paquete signal.' }
$oldPath = $env:PATH
try {
    $env:PATH = $octaveBin + ';' + $env:PATH
    $octave = Join-Path $octaveBin 'octave-cli.exe'
    $scripts = (Join-Path $base 'Octave').Replace('\','/').Replace("'","''")
    & $octave --quiet --eval "addpath('$scripts'); disenar_filtro();"
    if ($LASTEXITCODE -ne 0) { throw 'Fallo el diseno.' }
    $build = Join-Path $base 'C/build'
    New-Item -ItemType Directory -Force $build | Out-Null
    foreach ($programa in @('timeconv','fastconv_rt')) {
        $exe = Join-Path $build "$programa.exe"
        & (Join-Path $octaveBin 'gcc.exe') -std=c11 -O2 -Wall -Wextra -Werror (Join-Path $base "C/$programa.c") -o $exe -lm
        if ($LASTEXITCODE -ne 0) { throw "Fallo compilacion $programa" }
        $metodo = if ($programa -eq 'timeconv') { 'temporal' } else { 'fft' }
        foreach ($senal in @('ruido','senos','impulso')) {
            & $exe $senal (Join-Path $base "Resultados/${senal}_${metodo}.csv") 8192
            if ($LASTEXITCODE -ne 0) { throw "Fallo $programa con $senal" }
        }
    }
    & $octave --quiet --eval "addpath('$scripts'); verificar_y_graficar();"
    if ($LASTEXITCODE -ne 0) { throw 'Fallo la verificacion.' }
} finally { $env:PATH = $oldPath }
