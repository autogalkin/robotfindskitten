
set shell := ["cmd", "/c"]

format:
    #!/usr/bin/env sh
    find ./src -name "*.cpp" -o -name "*.hpp" -o -name "*.h" | xargs -I {} clang-format --verbose -i {}

build-x64-debug:
    (where /q  cl  ||  IF ERRORLEVEL 1 \
      C:\"Program Files (x86)"\"Microsoft Visual Studio"\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat x86_amd64) \
    &&  cmake --build --preset x64-debug --target install
