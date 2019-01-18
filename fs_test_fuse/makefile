compile:
	gcc server_example.c -o server_example -D_FILE_OFFSET_BITS=64 -lcurl -pthread
	gcc client_example.c -o client_example -D_FILE_OFFSET_BITS=64
	gcc -Wall teste.c `pkg-config fuse3 --cflags --libs` -o teste

