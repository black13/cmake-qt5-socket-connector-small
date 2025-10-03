@echo off
REM Run the complete graph example script
REM This creates a graph with all 5 node types and 0 empty sockets

echo ================================================
echo Running Complete Graph Example
echo ================================================
echo.

REM Run NodeGraph with the complete graph script
build_Debug\Debug\NodeGraph.exe --script scripts\example_complete_graph.js

echo.
echo ================================================
echo Script execution completed
echo ================================================
echo.
echo Check these files:
echo - complete_graph.xml (the generated graph)
echo - logs\ (application logs)
echo.

pause
