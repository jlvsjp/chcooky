TARGET	:=	chcooky
SOURCE	:=	chcooky.c

macos: $(TARGET)_macos

$(TARGET)_macos: chcooky.c libchcooky.go
	go build -x -ldflags "-s -w" -buildmode=c-archive -o libchcooky.a libchcooky.go
	gcc $(SOURCE) -D__MACOSX__ -L. -lchcooky -o $(TARGET)_macos -framework CoreFoundation -framework Security
	strip $(TARGET)_macos
	mv $(TARGET)_macos ./bin/$(TARGET)_macos
	rm libchcooky.a
	rm libchcooky.h

windows: $(TARGET).exe

$(TARGET).exe: chcooky.c libchcooky.go
	go env -w GOARCH=386
	go build -x -ldflags "-s -w" -buildmode=c-archive -o libchcooky.a libchcooky.go
	gcc -m32 -o $(TARGET).exe $(SOURCE) libchcooky.a
	strip $(TARGET).exe
	cmd /c move $(TARGET).exe .\\bin\\$(TARGET).exe
	cmd /c del libchcooky.a
	cmd /c del libchcooky.h

linux: $(TARGET)_linux

$(TARGET)_linux:
	go build -x -ldflags "-s -w" -buildmode=c-archive -o libchcooky.a libchcooky.go
	gcc -o $(TARGET)_linux $(SOURCE) libchcooky.a
	strip $(TARGET)_linux
	mv $(TARGET)_linux ./bin/$(TARGET)_linux
	rm libchcooky.a
	rm libchcooky.h
