compile: 
	clang++ -shared -o ./Code/libjobsystem.so -fPIC ./Code/lib/*.cpp
	clang++ -o a ./Code/*.cpp -L./Code/ -ljobsystem -Wl,-rpath,./Code/