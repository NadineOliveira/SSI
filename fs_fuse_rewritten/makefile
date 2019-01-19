run:
	./compile.sh
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/x86_64-linux-gnu
	export LD_LIBRARY_PATH
	mkdir -p ~/Desktop/teste
	./criaNovaJanela.sh
	./teste -s -f ~/Desktop/teste

help:
	echo "se tiver problemas com as bibliotecas por favor siga as instruções que foram dadas na consola de pois de correr o make run"
	echo "é possivel que no envio de email ocorra uma filtragem para spam"
	echo "se tal acontecer, por favor vá lá para retirar o código de acesso"
	echo "se tiver com problemas em mandar emails a solução deve ser fazer login para o email da gmail que está a ser usado"
	echo "utilizador: tp3ssiezenad ; palavra-passe: palavrapasse2"
	echo "se ainda persistir por favor reeinicie o computador com o seu browser"
	echo "se mesmo assim ainda persistir por favor reeinicie o computador sem sair da sessão e tente novamente"
stop:
	rm -r ~/Desktop/teste

