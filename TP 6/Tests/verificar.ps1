$ErrorActionPreference = 'Stop'
$octaveBin = Get-ChildItem -LiteralPath "$env:ProgramFiles/GNU Octave" -Directory |
    Sort-Object Name -Descending | ForEach-Object { Join-Path $_.FullName 'mingw64/bin' } |
    Where-Object { Test-Path (Join-Path $_ 'octave-cli.exe') } | Select-Object -First 1
if (-not $octaveBin) { throw 'No se encontro GNU Octave en Program Files.' }
# GCC y el simulador necesitan las DLL distribuidas con Octave.
$pathAnterior = $env:PATH
$env:PATH = $octaveBin + ';' + $env:PATH
try {
    $inc = Join-Path $PSScriptRoot '../STM32/TP5-DSP/Core/Inc'
    $octaveScripts = (Join-Path $PSScriptRoot '../Octave').Replace('\', '/').Replace("'", "''")
    & (Join-Path $octaveBin 'octave-cli.exe') --quiet --eval "addpath('$octaveScripts'); disenar_iir(false);"
    if ($LASTEXITCODE -ne 0) { throw 'Fallo el diseno de los IIR.' }
    # Compilar DESPUES de generar el encabezado para probar el diseno vigente.
    & (Join-Path $octaveBin 'gcc.exe') -std=c11 -O2 -Wall -Wextra -Werror "-I$inc" (Join-Path $PSScriptRoot 'iir_sim.c') -o (Join-Path $PSScriptRoot 'iir_sim.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Fallo la compilacion del simulador C.' }
    & (Join-Path $octaveBin 'octave-cli.exe') --quiet --eval "addpath('$octaveScripts'); verificar_iir(false);"
    if ($LASTEXITCODE -ne 0) { throw 'Fallo la verificacion Octave/C.' }
} finally {
    $env:PATH = $pathAnterior
}
