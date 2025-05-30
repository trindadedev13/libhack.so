rm -rf build && mkdir build && cd build

cmake .. && make

cp libhack.so ../sharedLibPreCompiled