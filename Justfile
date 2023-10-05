
set shell := ["cmd", "/c"]

format:
    #!/usr/bin/env sh
    find ./src -name "*.cpp" -o -name "*.hpp" -o -name "*.h" | xargs -I {} clang-format -style=file --verbose -i {}


build-x64-debug:
    (where /q  cl  ||  IF ERRORLEVEL 1 \
      C:\"Program Files (x86)"\"Microsoft Visual Studio"\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat x86_amd64) > NUL  \
    &&  cmake --build --preset x64-debug --target install

build-x64-release:
    (where /q  cl  ||  IF ERRORLEVEL 1 \
      C:\"Program Files (x86)"\"Microsoft Visual Studio"\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat x86_amd64) > NUL  \
    &&  cmake --build --preset x64-release --target install

configure-x64-debug:
    (where /q  cl  ||  IF ERRORLEVEL 1 \
       C:\"Program Files (x86)"\"Microsoft Visual Studio"\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat x86_amd64) > NUL \
    &&  cmake . --preset x64-debug

configure-x64-release:
    (where /q  cl  ||  IF ERRORLEVEL 1 \
       C:\"Program Files (x86)"\"Microsoft Visual Studio"\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat x86_amd64) > NUL \
    &&  cmake . --preset x64-release

run-debug:
    .\out\bin\x64-debug\loader.exe 

build-run-debug: build-x64-debug && run-debug

run-release:
    .\out\bin\x64-debug\loader.exe 

lldb:
    lldb -s .lldb .\out\bin\x64-debug\loader.exe 

lldb-notepad:
    lldb -n notepad.exe


build-scintilla-debug:
    (where /q  cl  ||  IF ERRORLEVEL 1 \
       C:\"Program Files (x86)"\"Microsoft Visual Studio"\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat x86_amd64) > NUL \
    &&  cmake --build . --target scintilla --preset x64-debug 

build-scintilla-release:
    (where /q  cl  ||  IF ERRORLEVEL 1 \
       C:\"Program Files (x86)"\"Microsoft Visual Studio"\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat x86_amd64) > NUL \
    &&  cmake --build . --target scintilla --preset x64-debug 

test:
    (where /q  cl  ||  IF ERRORLEVEL 1 \
       C:\"Program Files (x86)"\"Microsoft Visual Studio"\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat x86_amd64) > NUL \
    &&  cmake --build . --target test --preset x64-debug 

build-all:
    (where /q  cl  ||  IF ERRORLEVEL 1 \
       C:\"Program Files (x86)"\"Microsoft Visual Studio"\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat x86_amd64) > NUL \
    &&  cmake --build . --target all install --preset x64-debug 
    
