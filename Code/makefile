.SILENT:

libLinux:
	clang++ -shared -o ./lib/libjobsystem.so -fPIC ./lib/*.cpp

libWindows:
	g++ -shared -o ./libjobsystem.dll ./lib/*.cpp -Wl,--out-implib,./libjobsystem.a

buildLinux:
	clang++ -o a *.cpp -L./lib -ljobsystem -Wl,-rpath,./lib

buildWindows:
	g++ -o a *.cpp -L./ -ljobsystem
	./a

runWindows:
	g++ -shared -o ./libjobsystem.dll ./lib/*.cpp -Wl,--out-implib,./libjobsystem.a
	g++ -o a *.cpp -L./ -ljobsystem
	./a

# ALL LIB FILES WILL BE NAMED: lib[name].dll
# THE BUILD COMMAND ARGUMENT HAS TO BE: -l[name]