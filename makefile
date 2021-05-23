All: jpeg2json.exe

CFLAGS=/D_CRT_SECURE_NO_WARNINGS=1 /O2 /nologo /GL /W4 /MD /I./ /arch:AVX2 /std:c17 /Zi /Qvec-report:1

CXXFLAGS=/D_CRT_SECURE_NO_WARNINGS=1 /O2 /nologo /GL /W4 /MD /EHsc /I./ /fp:fast /arch:AVX2 /Zc:__cplusplus /std:c++17 /Zi /permissive- /experimental:external /external:anglebrackets /external:W0

CLANGFLAGS=-std=c++20 -Wall --extra-warnings --pedantic -mavx2 -ffp-contract=fast -fopenmp

jpeg2json.exe: $*.cpp jpegparsers.hpp ../pc10/pc10.hpp
	$(CC) $*.cpp $(CXXFLAGS) /Fe:$@
