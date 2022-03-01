cp -r /mnt/Shared-Folder/DOS-Emulator/* ./
../emcc -o index.html -s FETCH=1 -s ASYNCIFY -s NO_EXIT_RUNTIME=0 -s INITIAL_MEMORY=500MB -s ALLOW_MEMORY_GROWTH=1 --preload-file examples -fno-rtti -fno-exceptions -O3 --profiling ./src/main.cpp ./src/emulator.cpp ./src/bridge.cpp 
cp /mnt/Shared-Folder/DOS-Emulator/index.html ./index.html
python3 -m http.server
