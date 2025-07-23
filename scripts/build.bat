@echo off
echo Building LAN Chat application...

REM Create build directory
if not exist build mkdir build

REM Run CMake
echo Running CMake configuration...
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo Error: CMake configuration failed
    pause
    exit /b 1
)

REM Build the project
echo Building the project...
cmake --build build --config Release
if errorlevel 1 (
    echo Error: Build failed
    pause
    exit /b 1
)

REM Copy executable to root directory for easy access
if exist "build\Release\lanchat.exe" (
    copy "build\Release\lanchat.exe" .\
    echo.
    echo ✓ Build successful!
    echo ✓ Executable created: lanchat.exe
) else if exist "build\lanchat.exe" (
    copy "build\lanchat.exe" .\
    echo.
    echo ✓ Build successful!
    echo ✓ Executable created: lanchat.exe
) else (
    echo Error: Executable not found after build
    pause
    exit /b 1
)

echo.
echo To run the application:
echo   lanchat.exe
echo   (or lanchat.exe --port 8888 to use a different port)
echo.
echo Then open your browser and go to:
echo   http://localhost:8080
echo.
pause