Nome: Michelle Wingter da Silva, nUSP: 10783243

**Este Trabalho contém os 3 módulos:**

* Módulo 1 - Implementação de Sockets
* Módulo 2 - Comunicação entre múltiplos clientes e servidor
* **Módulo 3 - Implementação de múltiplos canais**


# Especificação

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

Nesta aplicação é possivel criar um servidor em uma porta escolhida pelo usuario, e conectar 
multiplos clientes na porta aberta. Além disso, o cliente pode criar/administrar/entrar em canais.

Especificações do servidor: Ele aceita mensagens de até 4096. Caso o cliente envie uma mensagem 
maior que este valor, a conexão com o cliente será terminada. Foi decidido fazer desta forma por 
uma questão de performance do servidor, pois assim ele apenas recebe e imprime mensagens, ficando 
a cargo do cliente dividir mensagens grandes.	
Caso queira testar esse funcionamento, pode utilizar o nc (ncat, telnet, etc), conectando no IP e 
porta do servidor, colocando o nome, e em seguida enviando uma mensagem maior que 4096. Este teste 
não pode ser realizado em nosso cliente pois ele já segue as especificações (RFC) do servidor.


## COMO EXECUTAR

* **Para compilar o programa, digite:**

	*"make all"*

* **Para criar o servidor (neste caso, será criado o servidor que estará ouvindo na porta 1234), digite:**

	*"make server"*

	(Ou, para escolher outra porta, digite: *"./bin/server <numero_da_porta>"* )

* **Para criar o cliente (cada um em um terminal diferente - se conectara na porta 1234), digite:**

	*"make client"*

	(Ou, para se conectar em uma porta diferente, digite: *"./bin/client <numero_da_porta>"* )

	Em seguida, siga as instruções, e estará pronto para conversar no chat.



## OUTPUT

* **No terminal do SERVIDOR, ao executar o programa, seu servidor estará criado e aparecerá a mensagem:**

" === NOVO CHAT [PORTA <numero_da_porta>] CRIADO ===

< conversa do chat aparecerá aqui >
"


* **No terminal do CLIENTE, após executar o programa, o cliente receberá a seguinte mensagem:**
  
  "
  - Para conectar ao servidor, digite: /connect
  - Para escolher um nickname, digite: /nickname <nickname_desejado>
  - Para sair, digite: /quit ou pressione Ctrl + D
  
  "

  Após o cliente dar o comando "/connect", ele entrará no servidor (na porta escolhida ao executar o programa), e receberá as seguintes mensagens e instruções:

  "
  
  === OLÁ! BEM-VINDO AO CHAT [PORTA <numero_da_porta>] ===
  _____________________________________________________________________________________________
    INSTRUÇÕES:
   - Para mandar uma mensagem, basta digitar ao lado do simbolo '>' abaixo e teclar Enter
   - Para escolher um nickname, digite: /nickname <Nickname_Desejado>
   - Para entrar em um canal, digite: /join <Nome_do_Canal>
   - Digite /ping para receber do servidor um retorno 'pong' assim que este receber a mensagem.
   - Para sair do chat, digite: /quit ou pressione Ctrl + D
  _____________________________________________________________________________________________

     .> <aqui será onde poderão ser enviadas mensagens no chat, e onde aparecerão as mensagens recebidas>
     
  "

	* Se, por exemplo, o Cliente-0 digitar o comando para entrar em um canal (/join <Nome_do_Canal>):
	
		* Se ele não for o primeiro cliente a entrar no canal escolhido, receberá a seguinte mensagem:
	
          "========================================
          
          Você entrou no canal <Nome_Do_Canal>

          =========================================
          
          ----- Admin do Canal <Nome_Do_Canal>: <Nome_Do_Admin>, ID: <ID_Do_Admin> ------
          
          [ - Para sair do canal, digite: /leavechannel ]
          
          "

		* Se ele for o primeiro cliente a entrar no canal escolhido, ele será o administrador deste canal e receberá, além da mensagem acima, as seguintes mensagens e instruções:

      "
      
      ***** COMANDOS DO ADMIN *****

      /kick <Nickname> - Fecha a conexão do usuário especificado
      
      /mute <Nickname> - Muta o usuário para que não possa ouvir mensagens neste canal;
      
      /unmute <Nickname> - Retira o mute de um usuário;
      
      /whois <Nickname> - Retorna o endereço IP do usuário apenas para o admin.

      "

