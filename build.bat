@echo off
setlocal EnableDelayedExpansion

echo ==========================================
echo Building UC-Online DLLs (x86 and x64)
echo ==========================================
echo.

:: ============================================================================
:: Compiler selection: pass "gcc" as first argument to use MinGW-w64 / GCC.
:: Default (no argument) uses MSVC / Visual Studio.
::
::   build.bat          -> MSVC build
::   build.bat gcc      -> MinGW-w64 / GCC build (requires MSYS2 or standalone MinGW-w64)
::
:: For the GCC path the script looks for the MSYS2 toolchains at:
::   32-bit: C:\msys64\mingw32\bin\gcc.exe   (MINGW32 environment)
::   64-bit: C:\msys64\mingw64\bin\gcc.exe   (MINGW64 environment)
::
:: If MSYS2 is not at C:\msys64 you can override the paths:
::   set MINGW32_BIN=C:\path\to\mingw32\bin
::   set MINGW64_BIN=C:\path\to\mingw64\bin
:: ============================================================================

set "USE_GCC=0"
if /i "%~1"=="gcc" set "USE_GCC=1"

if "%USE_GCC%"=="1" goto :build_gcc

:: ============================================================================
:: MSVC / Visual Studio build
:: ============================================================================

:: Check for Visual Studio installation
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: Visual Studio not found. Please install Visual Studio with C++ support.
    exit /b 1
)

:: Detect latest Visual Studio version
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do (
    set "VS_PATH=%%i"
)

if not defined VS_PATH (
    echo ERROR: Could not find Visual Studio installation.
    exit /b 1
)

echo Found Visual Studio at: %VS_PATH%

:: Try to detect VS version for CMake generator
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property displayName`) do (
    set "VS_NAME=%%i"
)
echo Visual Studio: %VS_NAME%

:: Determine CMake generator based on VS version
set "CMAKE_GEN="
echo %VS_NAME% | findstr "2022" >nul && set "CMAKE_GEN=Visual Studio 17 2022"
echo %VS_NAME% | findstr "2019" >nul && set "CMAKE_GEN=Visual Studio 16 2019"
echo %VS_NAME% | findstr "2017" >nul && set "CMAKE_GEN=Visual Studio 15 2017"

if not defined CMAKE_GEN (
    echo WARNING: Unknown VS version, trying Visual Studio 17 2022
    set "CMAKE_GEN=Visual Studio 17 2022"
)

echo Using CMake generator: %CMAKE_GEN%
echo.

:: Clean previous builds if they exist
if exist build-x86 (
    echo Cleaning previous x86 build...
    rmdir /s /q build-x86
)
if exist build-x64 (
    echo Cleaning previous x64 build...
    rmdir /s /q build-x64
)

:: Build x86 version
echo.
echo ==========================================
echo Building 32-bit (x86) version...
echo ==========================================
mkdir build-x86
cd build-x86

echo Running CMake for x86...
cmake -G "%CMAKE_GEN%" -A Win32 ..
if errorlevel 1 (
    echo ERROR: CMake configuration failed for x86
    cd ..
    exit /b 1
)

echo Building x86 Release...
cmake --build . --config Release
if errorlevel 1 (
    echo ERROR: Build failed for x86
    cd ..
    exit /b 1
)

cd ..
echo x86 build completed successfully!

:: Build x64 version
echo.
echo ==========================================
echo Building 64-bit (x64) version...
echo ==========================================
mkdir build-x64
cd build-x64

echo Running CMake for x64...
cmake -G "%CMAKE_GEN%" -A x64 ..
if errorlevel 1 (
    echo ERROR: CMake configuration failed for x64
    cd ..
    exit /b 1
)

echo Building x64 Release...
cmake --build . --config Release
if errorlevel 1 (
    echo ERROR: Build failed for x64
    cd ..
    exit /b 1
)

cd ..
echo x64 build completed successfully!

goto :copy_output

:: ============================================================================
:: MinGW-w64 / GCC build
:: ============================================================================
:build_gcc

echo Using MinGW-w64 / GCC toolchain
echo.

:: ---- Locate 32-bit (MINGW32) toolchain ----
:: The 32-bit target MUST use the i686 (MINGW32) toolchain.
:: Using the x86_64 (MINGW64) toolchain with -m32 does NOT work because
:: the 64-bit toolchain ships only 64-bit import libraries.

if not defined MINGW32_BIN (
    if exist "C:\msys64\mingw32\bin\gcc.exe" (
        set "MINGW32_BIN=C:\msys64\mingw32\bin"
    ) else (
        echo WARNING: C:\msys64\mingw32\bin\gcc.exe not found.
        echo          Set MINGW32_BIN to the directory containing the 32-bit gcc.exe
        echo          e.g.  set MINGW32_BIN=C:\mingw32\bin
        echo          Skipping 32-bit GCC build.
        set "SKIP_X86=1"
    )
)

:: ---- Locate 64-bit (MINGW64) toolchain ----
if not defined MINGW64_BIN (
    if exist "C:\msys64\mingw64\bin\gcc.exe" (
        set "MINGW64_BIN=C:\msys64\mingw64\bin"
    ) else (
        echo WARNING: C:\msys64\mingw64\bin\gcc.exe not found.
        echo          Set MINGW64_BIN to the directory containing the 64-bit gcc.exe
        echo          e.g.  set MINGW64_BIN=C:\mingw64\bin
        echo          Skipping 64-bit GCC build.
        set "SKIP_X64=1"
    )
)

:: ---- GCC x86 (32-bit) ----
if not defined SKIP_X86 (
    echo.
    echo ==========================================
    echo Building 32-bit (x86) with GCC (MINGW32)...
    echo ==========================================

    if exist build-gcc-x86 (
        echo Cleaning previous GCC x86 build...
        rmdir /s /q build-gcc-x86
    )
    mkdir build-gcc-x86
    cd build-gcc-x86

    echo Running CMake for GCC x86...
    echo   Compiler: %MINGW32_BIN%\gcc.exe
    cmake -G "MinGW Makefiles" ^
          -DCMAKE_BUILD_TYPE=Release ^
          -DCMAKE_C_COMPILER="%MINGW32_BIN%\gcc.exe" ^
          -DCMAKE_CXX_COMPILER="%MINGW32_BIN%\g++.exe" ^
          -DCMAKE_RC_COMPILER="%MINGW32_BIN%\windres.exe" ^
          ..
    if errorlevel 1 (
        echo ERROR: CMake configuration failed for GCC x86
        cd ..
        exit /b 1
    )

    cmake --build .
    if errorlevel 1 (
        echo ERROR: GCC x86 build failed
        cd ..
        exit /b 1
    )

    cd ..
    echo GCC x86 build completed successfully!
)

:: ---- GCC x64 (64-bit) ----
if not defined SKIP_X64 (
    echo.
    echo ==========================================
    echo Building 64-bit (x64) with GCC (MINGW64)...
    echo ==========================================

    if exist build-gcc-x64 (
        echo Cleaning previous GCC x64 build...
        rmdir /s /q build-gcc-x64
    )
    mkdir build-gcc-x64
    cd build-gcc-x64

    echo Running CMake for GCC x64...
    echo   Compiler: %MINGW64_BIN%\gcc.exe
    cmake -G "MinGW Makefiles" ^
          -DCMAKE_BUILD_TYPE=Release ^
          -DCMAKE_C_COMPILER="%MINGW64_BIN%\gcc.exe" ^
          -DCMAKE_CXX_COMPILER="%MINGW64_BIN%\g++.exe" ^
          -DCMAKE_RC_COMPILER="%MINGW64_BIN%\windres.exe" ^
          ..
    if errorlevel 1 (
        echo ERROR: CMake configuration failed for GCC x64
        cd ..
        exit /b 1
    )

    cmake --build .
    if errorlevel 1 (
        echo ERROR: GCC x64 build failed
        cd ..
        exit /b 1
    )

    cd ..
    echo GCC x64 build completed successfully!
)

goto :copy_output

:: ============================================================================
:: Copy output files
:: ============================================================================
:copy_output

echo.
echo ==========================================
echo Copying built files to output directory...
echo ==========================================

if not exist output mkdir output
if not exist output\x86 mkdir output\x86
if not exist output\x64 mkdir output\x64

if "%USE_GCC%"=="1" (
    :: GCC output paths (flat, no Release sub-directory)
    if not defined SKIP_X86 (
        copy /y build-gcc-x86\steam_api.dll output\x86\ 2>nul
    )
    if not defined SKIP_X64 (
        copy /y build-gcc-x64\steam_api64.dll output\x64\ 2>nul
    )
) else (
    :: MSVC output paths
    copy /y build-x86\Release\steam_api.dll   output\x86\
    copy /y build-x64\Release\steam_api64.dll output\x64\
)

echo.
echo ==========================================
echo Build Summary
echo ==========================================
echo.
echo x86 (32-bit) files in: output\x86\
echo   - steam_api.dll  (rename original to union-crax.dll in game dir)
echo.
echo x64 (64-bit) files in: output\x64\
echo   - steam_api64.dll  (rename original to union-crax64.dll in game dir)
echo.
echo Installation instructions:
echo   1. Rename the game's original steam_api.dll   to union-crax.dll
echo   2. Rename the game's original steam_api64.dll to union-crax64.dll
echo   3. Copy the built steam_api.dll / steam_api64.dll into the game directory
echo   4. Place config.ini next to the DLL in the game directory
echo.
echo ==========================================
echo Build completed successfully!
echo ==========================================

endlocal
