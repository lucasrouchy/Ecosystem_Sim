proj2:         proj2.cpp
	/usr/local/Cellar/gcc/12.2.0/bin/g++-12     -lomp proj2.cpp   -o proj2 -I /usr/local/opt/libomp/include/ -L /usr/local/opt/libomp/lib/    -fopenmp