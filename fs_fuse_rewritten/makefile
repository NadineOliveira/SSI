run:
	./compile.sh
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/x86_64-linux-gnu
	export LD_LIBRARY_PATH
	if [ -d "~/Desktop/teste/" ]; then echo "folder already exists" else mkdir "~/Desktop/teste/"; fi
	./inicializaCliente.sh 
	./teste -s ~/Desktop/teste/
