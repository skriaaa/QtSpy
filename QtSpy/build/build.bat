cd /d %~dp0..
del "CMakeCache.txt"
rmdir /s /q "CMakeFiles"
cmake . -B build -DPLATFORMTYPE:STRING=Windows -A Win32