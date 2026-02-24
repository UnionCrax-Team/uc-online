name: Build Steam Proxy DLLs

on:
  push:
    branches: [ "dllsteamapi" ]
  pull_request:
    branches: [ "dllsteamapi" ]

jobs:

# ======================================================
# MinGW-w64 build (static runtime)
# ======================================================

  mingw:

    runs-on: windows-latest

    strategy:
      matrix:
        arch: [x86, x64]

    steps:

      - name: Checkout
        uses: actions/checkout@v4

      - name: Setup MinGW
        uses: egor-tensin/setup-mingw@v2
        with:
          platform: ${{ matrix.arch }}

      - name: Configure (MinGW)
        run: |
          cmake -S . -B build `
            -G "MinGW Makefiles" `
            -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: cmake --build build --parallel

      - name: Verify dependencies
        run: |
          objdump -p build/*.dll | findstr "DLL Name"

      - name: Upload artifacts
        uses: actions/upload-artifact@v4
        with:
          name: mingw_${{ matrix.arch }}
          path: build/*.dll


# ======================================================
# MSVC build (Visual Studio toolchain)
# ======================================================

  msvc:

    runs-on: windows-latest

    strategy:
      matrix:
        arch: [Win32, x64]

    steps:

      - name: Checkout
        uses: actions/checkout@v4

      - name: Setup MSVC Dev Environment
        uses: ilammy/msvc-dev-cmd@v1
        with:
          arch: ${{ matrix.arch }}

      - name: Configure (MSVC)
        run: |
          cmake -S . -B build `
            -G "Visual Studio 17 2022" `
            -A ${{ matrix.arch }}

      - name: Build
        run: |
          cmake --build build --config Release --parallel

      - name: Upload artifacts
        uses: actions/upload-artifact@v4
        with:
          name: msvc_${{ matrix.arch }}
          path: build/Release/*.dll
