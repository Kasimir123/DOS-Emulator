cp /mnt/Shared-Folder/DOS-Emulator/* ./
../emcc -o index.html -s FETCH=1 -s ASYNCIFY -s NO_EXIT_RUNTIME=0 main.cpp emulator.cpp
cp /mnt/Shared-Folder/DOS-Emulator/index.html ./index.html
python3 -m http.server
