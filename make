cp /mnt/Shared-Folder/DOS-Emulator/* ./
../emcc -o main.html -s FETCH=1 -s ASYNCIFY -s NO_EXIT_RUNTIME=0 main.cpp emulator.cpp --preload-file MOV.EXE
cp /mnt/Shared-Folder/DOS-Emulator/main.html ./main.html
python3 -m http.server
