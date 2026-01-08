$ErrorActionPreference = "Stop"

$BinPath = Join-Path $PSScriptRoot "..\bin\MemoryManagementSimulator.exe"
$TestDir = $PSScriptRoot
$Tests = @("workload_allocation", "workload_cache")

if (-not (Test-Path $BinPath)) {
    Write-Error "Simulator binary not found at $BinPath. Please build the project first."
}

Write-Host "Running Tests..." -ForegroundColor Cyan

foreach ($test in $Tests) {
    $InputFile = Join-Path $TestDir "$test.txt"
    $OutputFile = Join-Path $TestDir "$test.out"
    
    Write-Host "  Executing $test..." -NoNewline
    
    try {
        Get-Content $InputFile | & $BinPath > $OutputFile
        Write-Host " DONE" -ForegroundColor Green
        Write-Host "    Output saved to $OutputFile" -ForegroundColor Gray
    } catch {
        Write-Host " FAILED" -ForegroundColor Red
        Write-Error $_
    }
}

Write-Host "`nAll tests completed." -ForegroundColor Cyan
