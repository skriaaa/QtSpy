cd /d %~dp0..
del "CMakeCache.txt"
rmdir /s /q "CMakeFiles"
cmake . -DPLATFORMTYPE:STRING=Windows