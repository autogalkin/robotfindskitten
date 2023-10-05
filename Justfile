
set shell := ["cmd", "/c"]
set ignore-comments := true

# Show all presets
default:
    just --list

# format all sources with clang-format
format:
    #!/usr/bin/env sh
    find ./src -name "*.cpp" -o -name "*.hpp" -o -name "*.h" | xargs -I {} clang-format -style=file --verbose -i {}

vcvarsall  := '(where /q  cl ||  IF ERRORLEVEL 1 C:\"Program Files (x86)"\"Microsoft Visual Studio"\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat x86_amd64) > NUL'

#configure with x64-debug preset
[windows]
configure-x64-debug:
    {{vcvarsall}} &&  cmake . --preset x64-debug

#configure with x64-release preset
[windows]
configure-x64-release:
    {{vcvarsall}} &&  cmake . --preset x64-release

#build with x64-debug preset
[windows]
build-x64-debug:
    {{vcvarsall}} &&  cmake --build --preset x64-debug --target install

#build with x64-release preset
[windows]
build-x64-release:
    {{vcvarsall}} &&  cmake --build --preset x64-release --target install


#build only patched scintilla dll
[windows]
build-scintilla:
    {{vcvarsall}} &&  cmake --build . --target scintilla

#perform all tests
[windows]
test:
    {{vcvarsall}} &&  cmake --build . --target test --preset x64-debug 

#build game and scintilla and perform all tests
[windows]
build-all:
    {{vcvarsall}} &&  cmake --build . --target all install --preset x64-debug 

#run debug build game
[windows]
run-debug:
    .\out\bin\x64-debug\loader.exe 

#build and run with x64-debug preset
[windows]
build-run-debug: build-x64-debug && run-debug

#run release build game
[windows]
run-release:
    .\out\bin\x64-debug\loader.exe 

#run lldb with .llfb script for stop at injection
[windows]
lldb:
    lldb -s .lldb .\out\bin\x64-debug\loader.exe 

#connect lldb to notepad
[windows]
lldb-notepad:
    lldb -n notepad.exe
    
