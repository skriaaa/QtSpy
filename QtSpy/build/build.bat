del "CMakeCache.txt"
rmdir /s /q "CMakeFiles"
cd /d %~dp0..
cmake . -B build -DPLATFORMTYPE:STRING=Windows -A Win32