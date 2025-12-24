```bash
mkdir build && mkdir build/debug || mkdir build/debug && cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..

mkdir build && mkdir build/release || mkdir build/release && cd build/release
cmake -DCMAKE_BUILD_TYPE=Release ../..

make

./flexa
```
