# PowerShell script to run the complete graph example
# This creates a graph with all 5 node types and 0 empty sockets

Write-Host "================================================" -ForegroundColor Cyan
Write-Host "Running Complete Graph Example" -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan
Write-Host ""

# Check if executable exists
if (!(Test-Path "build_Debug\Debug\NodeGraph.exe")) {
    Write-Host "ERROR: NodeGraph.exe not found!" -ForegroundColor Red
    Write-Host "Please build the project first:" -ForegroundColor Yellow
    Write-Host "  cd build_Debug" -ForegroundColor Yellow
    Write-Host "  msbuild NodeGraph.sln /p:Configuration=Debug" -ForegroundColor Yellow
    exit 1
}

# Check if script exists
if (!(Test-Path "scripts\example_complete_graph.js")) {
    Write-Host "ERROR: Script not found!" -ForegroundColor Red
    Write-Host "Expected: scripts\example_complete_graph.js" -ForegroundColor Yellow
    exit 1
}

Write-Host "Launching NodeGraph with complete graph script..." -ForegroundColor Green
Write-Host ""

# Run the application with the script
& ".\build_Debug\Debug\NodeGraph.exe" --script "scripts\example_complete_graph.js"

Write-Host ""
Write-Host "================================================" -ForegroundColor Cyan
Write-Host "Script execution completed" -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan
Write-Host ""

# Check for output files
if (Test-Path "complete_graph.xml") {
    Write-Host "✓ Graph saved: complete_graph.xml" -ForegroundColor Green
    Write-Host ""
    Write-Host "Preview of saved graph:" -ForegroundColor Yellow
    Get-Content "complete_graph.xml" | Select-Object -First 20
    Write-Host "..." -ForegroundColor Gray
} else {
    Write-Host "⚠ complete_graph.xml not found" -ForegroundColor Yellow
}

Write-Host ""

# Check for log files
if (Test-Path "logs\") {
    $latestLog = Get-ChildItem "logs\JavaScript_*.log" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($latestLog) {
        Write-Host "✓ JavaScript log: $($latestLog.Name)" -ForegroundColor Green
        Write-Host ""
        Write-Host "Last 30 lines of log:" -ForegroundColor Yellow
        Get-Content $latestLog.FullName | Select-Object -Last 30
    }
} else {
    Write-Host "⚠ logs\ directory not found" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "To view the graph in the GUI:" -ForegroundColor Cyan
Write-Host "  .\build_Debug\Debug\NodeGraph.exe --load complete_graph.xml" -ForegroundColor White
