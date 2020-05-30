Nome: Michelle Wingter da Silva
nUSP: 10783243

# Módulo 1 - Implementação de Sockets (entrega 26/04/2020)

## Especificação da implementação

Imagine uma aplicação online como um chat ou um jogo. Como a comunicação dela é feita?
Neste módulo será desenvolvido uma aplicação para a comunicação entre Clientes na linguagem
C ou C++, sem o uso de bibliotecas externas.

Para isso, deve ser implementado um socket, que define um mecanismo de troca de dados
entre dois ou mais processos distintos, podendo estes estar em execução na mesma máquina ou
em máquinas diferentes, porém ligadas através da rede. Uma vez estabelecida a ligação entre dois
processos, eles devem poder enviar e receber mensagens um do outro.

Na aplicação a ser entregue devem ser implementados sockets TCP que permitam a comunicação
entre duas aplicações, isso de modo que o usuário da aplicação 2 possa ler e enviar mensagens para
o usuário da aplicação 1 e vice-versa.

O limite para o tamanho de cada mensagem deve ser de 4096 caracteres. Caso um usuário envie
uma mensagem maior do que isso ela deverá ser dividida em múltiplas mensagens automaticamente.


## FUNCIONALIDADES

Nesta aplicação é possivel criar um servidor em uma porta escolhida pelo usuario, e conectar multiplos clientes na porta aberta.
Especificações do servidor: Ele aceita mensagens de até 4096. Caso o cliente envie uma mensagem maior que este valor, a conexão com o cliente será terminada. Foi decidido fazer desta forma por uma questão de performance do servidor, pois assim ele apenas recebe e imprime mensagens, ficando a cargo do cliente dividir mensagens grandes.
	
Caso queira testar esse funcionamento, pode utilizar o nc (ncat, telnet, etc), conectando no IP e porta do servidor, colocando o nome, e em seguida enviando uma mensagem maior que 4096. Este teste não pode ser realizado em nosso cliente pois ele segue as especificações (RFC) do servidor.

## COMO USAR

* Para compilar o programa, digite:

	make all

* Para criar o servidor (neste caso, sera criado o servidor que estara ouvindo na porta 1234), digite:

	make server

	* Ou, para escolher a porta, digite: 
		./bin/server <numero_da_porta>

* Para criar o cliente (cada um em um terminal diferente - se conectara na porta 1234), digite:

	make client

	* Ou, para se conectar em uma porta diferente, digite:
		./bin/client <numero_da_porta>

	Em seguida, digite seu nome, envie, e estara pronto para conversar no chat.



## OUTPUT

* No terminal do Servidor:

" === NOVO CHAT [PORTA <numero_da_porta>] CRIADO ===

< conversa do chat aparecerá aqui >


* No terminal do Cliente:

" Digite seu nome: <digite aqui seu nome>

=== OLA, <nome>. BEM-VINDO AO CHAT [PORTA <numero_da_porta>] ===

< digite aqui suas mensagens a serem enviadas no char >

< conversa do chat aparecerá aqui > "

	
