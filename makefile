all:
	windres icon.rc -O coff -o icon.res
	gcc -Ofast icon.res rsa.c -o rsa.exe -lgmp `pkg-config --cflags --libs gtk+-3.0` `libgcrypt-config --cflags --libs` -mwindows
