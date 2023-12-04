.SILENT:

# Commands to run the actual code

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

# Commands for compiling the example projects

project1:
	-g++ -fdiagnostics-format=json -g -std=c++14 ../Data/compilecode/Project1/*.cpp -o ../Data/compilecode/Project1/auto_out1
project2:
	-g++ -fdiagnostics-format=json -g -std=c++14 ../Data/compilecode/Project2/*.cpp -o ../Data/compilecode/Project1/auto_out2
project3:
	-g++ -fdiagnostics-format=json -g -std=c++14 ../Data/compilecode/Project3/*.cpp -o ../Data/compilecode/Project1/auto_out3
project4:
	-g++ -fdiagnostics-format=json -g -std=c++14 ../Data/compilecode/Project4/*.cpp -o ../Data/compilecode/Project1/auto_out4

# ALL LIB FILES WILL BE NAMED: lib[name].dll
# THE BUILD COMMAND ARGUMENT HAS TO BE: -l[name]