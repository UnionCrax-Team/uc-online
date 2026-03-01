@echo off
setlocal

echo ==========================================
echo Building UC-Online with TCC (32-bit and 64-bit)
echo ==========================================
echo.

:: Create output directory
if not exist output\tcc mkdir output\tcc

:: ============================================================================
:: 32-bit build
:: ============================================================================
echo Building 32-bit (x86) version...
echo.

:: Compile resources for 32-bit
C:\msys64\mingw32\bin\windres.exe -i src\resources.rc -o output\tcc\resources32.res -D IS_64BIT=0 --input-format=rc --output-format=coff
if errorlevel 1 (
    echo ERROR: Resource compilation failed for 32-bit!
    exit /b 1
)

tcc\tcc\tcc.exe -shared -o output\tcc\steam_api.dll ^
    -I include -I tcc\include ^
    src\dllmain.c src\uc_online.c src\ini_config.c src\logger.c src\steam_api_stubs.c ^
    output\tcc\resources32.res ^
    -lkernel32 -luser32 -lgdi32 -lwinmm

if errorlevel 1 (
    echo ERROR: 32-bit build failed!
    exit /b 1
)

echo 32-bit build completed: output\tcc\steam_api.dll

:: ============================================================================
:: 64-bit build
:: ============================================================================
echo.
echo Building 64-bit (x64) version...
echo.

:: Compile resources for 64-bit
C:\msys64\mingw64\bin\windres.exe -i src\resources.rc -o output\tcc\resources64.res -D IS_64BIT=1 --input-format=rc --output-format=coff
if errorlevel 1 (
    echo ERROR: Resource compilation failed for 64-bit!
    exit /b 1
)

tcc\tcc\x86_64-win32-tcc.exe -shared -o output\tcc\steam_api64.dll ^
    -I include -I tcc\include ^
    src\dllmain.c src\uc_online.c src\ini_config.c src\logger.c src\steam_api_stubs.c ^
    output\tcc\resources64.res ^
    -lkernel32 -luser32 -lgdi32 -lwinmm

if errorlevel 1 (
    echo ERROR: 64-bit build failed!
    exit /b 1
)

echo 64-bit build completed: output\tcc\steam_api64.dll

:: ============================================================================
:: Summary
:: ============================================================================
echo.
echo ==========================================
echo Build Summary
echo ==========================================
echo.
echo 32-bit: output\tcc\steam_api.dll
echo 64-bit: output\tcc\steam_api64.dll
echo.
echo ==========================================
echo Build completed successfully!
echo ==========================================

endlocal

