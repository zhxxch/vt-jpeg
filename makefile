All: jpeg2json.exe

CFLAGS=/D_CRT_SECURE_NO_WARNINGS=1 /O2 /nologo /GL /W4 /MD /I./ /arch:AVX2 /std:c17 /Zi /Qvec-report:1

CXXFLAGS=/D_CRT_SECURE_NO_WARNINGS=1 /O2 /nologo /GL /W4 /MD /EHsc /I./ /fp:fast /arch:AVX2 /Zc:__cplusplus /std:c++17 /Zi

jpeg2json.exe: $*.cpp
	$(CC) $*.cpp $(CXXFLAGS) /Fe:$@
