Este projeto (fase 4 do projeto da cadeira de Sistemas Distribuídos)
foi desenvolvido pelo grupo 007, cujos elementos são:

    - André Dias    , nº 55314
    - David Pereira , nº 56361
    - Miguel Cut    , nº 56339

O projeto, que já conta com um modelo cliente-servidor(de projetos
anteriores), tem como objetivo implementar um suporte para tolerância a
falhas através de replicação do estado de um servidor, seguindo o
modelo Chain Replication (Replicação em Cadeia) e usando o serviço
de coordenação ZooKeeper.

Para a implementação do módulo tree.c escolhemos utilizar o ficheiro
providenciado pelos professores, sendo o respetivo tree.o incluido na
pasta object.

----------------->   Inicializar o ZoKeeper    <---------------------

Antes de correr o programa, é necessário ligar ao servidor ZooKeeper.
Para isso, deve-se abrir um terminal na pasta bin onde se encontra
instalado o ZooKeeper. De seguida, correr o comando:

zkServer.sh start

para inicializar o ZooKeeper.

--------------------------------------------------------------------

------------------->  Correr o programa  <--------------------------

Já dentro da pasta 'BST_Multi_Server_Multi_Client', para correr o
programa é necessário compilar o servidor e o cliente com o comando:

make

Depois executar o servidor com o comando:

./tree-server <port> <ip_ZooKeeper>:<port_ZooKeeper>

em que <port> é o número do porto TCP ao qual o servidor se deve associar
(fazer 'bind')

e em <ip_ZooKeeper>:<port_ZooKeeper>, o <ip_ZooKeeper> é o ip do servidor
ZooKeeper ao qual o servidor se deve conetar

e <port_ZooKeeper> corresponde à porta do ZooKeeper (2181)

-----------------------------------------------------------------------------

---------------------->         ATENÇÃO!        <----------------------------

Por motivos de segurança e para permitir a execução dos programas servidor
e cliente, a porta escolhida para o servidor deve ser maior ou igual a 1024
(<port> >= 1024)

Caso contrário será necessário será necessário correr o servidor como sudo:

sudo ./tree-server <port> <ip_ZooKeeper>:<port_ZooKeeper>

-----------------------------------------------------------------------------

E para o cliente correr no terminal:

./tree-client <ip_ZooKeeper>:<port_ZooKeeper>

em que <ip_ZooKeeper> corresponde ao endereço IP ou nome do servidor ZooKeeper
e <port_ZooKeeper> é o número do porto TCP onde o servidor ZooKeeper está à
espera de ligações

O programa cliente (tree-client) permitirá fazer algumas operações de alteração
e de look up na árvore guardada no servidor através de comandos específicos:

------> put <key> <data>        //  em que <key> corresponde à chave e <data>
                                    ao valor associado à chave que queremos
                                    inserir na árvore

------> get <key>               //  em que <key> corresponde à chave do valor
                                    que queremos obter da árvore

------> del <key>               //  em que <key> corresponde à chave da entrada
                                    que queremos apagar da árvore

------> size                    //  para obter o tamanho da árvore

------> height                  //  para obter a altura da árvore

------> getkeys                 //  para obter todas a keys da árvore

------> getvalues               //  para obter todos os valores da árvore

------> quit                    //  para encerrar o programa cliente

------> verify <id>             //  em que <id> corresponde ao id cujo pedido
                                    queremos verificar se já foi respondido ou
                                    ainda se encontra em espera

-----------------------------------------------------------------------------

--------------------------->        NOTA        <----------------------------

Ao fazer verify de um pedido que supostamente dá erro (por exemplo, delete de
uma key que não existe), o método verify devolve como resposta que o pedido
foi executado, uma vez que realmente foi executado, apenas não com sucesso.

-----------------------------------------------------------------------------
