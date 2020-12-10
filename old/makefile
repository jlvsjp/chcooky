TARGET=chcooky
SOURCE=chcooky.c

macos: $(TARGET)_macos

$(TARGET)_macos: $(SRC)
	gcc $(SOURCE) -D__MACOSX__ -lpthread -o $(TARGET)_macos
	strip $(TARGET)_macos
	mv $(TARGET)_macos ./bin/macos

windows: $(TARGET)_windows.exe

$(TARGET)_windows.exe: $(SRC)
	gcc $(SOURCE) -D__NT__ -lpthread -lwsock32 -static -o $(TARGET)_windows.exe
	mv $(TARGET)_windows.exe ./bin/windows 

linux: $(TARGET)_linux ./bin/$(TARGET)_linux_static

$(TARGET)_linux: $(SRC)
	gcc $(SOURCE) -lpthread -o $(TARGET)_linux_static
	strip $(TARGET)_linux
	mv $(TARGET)_linux ./bin/linux

$(TARGET)_linux_static: $(SRC)
	gcc $(SOURCE) -lpthread -static -o $(TARGET)_linux_static
	strip $(TARGET)_linux_static
	mv $(TARGET)_linux_static ./bin/linux

