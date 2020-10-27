TARGET	:=	chcooky
SOURCE	:=	chcooky.c

macos: $(TARGET)_macos

$(TARGET)_macos: chcooky.c libchcooky.go
	go build -x -v -ldflags "-s -w" -buildmode=c-archive -o libchcooky.a libchcooky.go
	gcc $(SOURCE) -D__MACOSX__ -L. -lchcooky -o $(TARGET)_macos -framework CoreFoundation -framework Security
	strip $(TARGET)_macos
	mv $(TARGET)_macos ./bin/$(TARGET)_macos
	rm libchcooky.a
	rm libchcooky.h

windows: $(TARGET)_windows.exe

$(TARGET).exe: chcooky.c libchcooky.go
	go env -w GOARCH=386
	go build -x -v -ldflags "-s -w" -buildmode=c-archive -o libchcooky.a libchcooky.go
	gcc -m32 -L. -lchcooky -o $(TARGET).exe $(SOURCE)
	strip $(TARGET).exe
	move $(TARGET).exe .\\bin\\$(TARGET).exe
	del libchcooky.a
	del libchcooky.h

linux: $(TARGET)_linux ./bin/$(TARGET)_linux_static

$(TARGET)_linux:
	go build -x -v -ldflags "-s -w" -buildmode=c-archive -o libchcooky.a libchcooky.go
	gcc $(SOURCE) -L. -lchcooky -o $(TARGET)_linux
	strip $(TARGET)_linux
	mv $(TARGET)_linux ./bin/$(TARGET)_linux
	rm libchcooky.a
	rm libchcooky.h
