#!/bin/bash
mkdir -p ../build
pushd ../build
CommonCompilerFlags=(-Wall -DSLOW_BUILD=1 -DINTERNAL_BUILD=1 -Werror -fno-rtti -Wl,-rpath,'$ORIGIN'
		     -Wno-unused-function -Wno-write-strings -Wno-unused-variable -g -Wno-null-dereference
		    -Wno-unused-but-set-variable)
CommonLinkerFlags=(-lGL -lGLEW `sdl2-config --cflags --libs` -ldl)

g++ ${CommonCompilerFlags[*]} -shared -fpic ../code/dwarves.cpp -o dwarves.so
g++ ${CommonCompilerFlags[*]} ../code/sdl_dwarves.cpp -o dwarves ${CommonLinkerFlags[*]}

#clang++ ${CommonCompilerFlags[*]} ../code/sdl_dwarves.cpp ${CommonLinkerFlags[*]}  -o dwarves #-Wl,-Map,linux32_output.map,--gc-sections

#clang++ ${CommonCompilerFlags[*]} -shared -o dwarves.so -fpic ../code/dwarves.cpp #-Wl,-Map,output.map,--gc-sections


popd

