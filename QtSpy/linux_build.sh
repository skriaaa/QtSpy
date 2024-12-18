rm "CMakeCache.txt"
rm -rf "CMakeFiles"
cmake . -DPLATFORMTYPE:STRING=Linux
make

