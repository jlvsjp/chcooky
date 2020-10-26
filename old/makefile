TARGET	:=	chcooky
SOURCE	:=	chcooky.c

macos: $(TARGET)_macos

$(TARGET)_macos:
	gcc $(SOURCE) -D__MACOSX__ -lpthread -o $(TARGET)_macos
	strip $(TARGET)_macos
	mv $(TARGET)_macos ./bin/macos

windows: $(TARGET)_windows.exe

$(TARGET)_windows.exe:
	gcc $(SOURCE) -D__NT__ -lpthread -lwsock32 -static -o $(TARGET)_windows.exe
	move $(TARGET)_windows.exe ./bin/windows

linux: $(TARGET)_linux ./bin/$(TARGET)_linux_static

$(TARGET)_linux:
	gcc $(SOURCE) -lpthread -o $(TARGET)_linux_static
	strip $(TARGET)_linux
	mv $(TARGET)_linux ./bin/linux

$(TARGET)_linux_static:
	gcc $(SOURCE) -lpthread -static -o $(TARGET)_linux_static
	strip $(TARGET)_linux_static
	mv $(TARGET)_linux_static ./bin/linux

