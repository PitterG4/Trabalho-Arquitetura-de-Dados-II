# Arquivos de Dados com Índices Parciais (AED II)

A ideia geral do trabalho é aplicar organização sequencial-indexada em arquivos binários usando dados reais de um e-commerce (retirados do Kaggle), sem carregar a base de dados inteira na memória RAM para fazer as operações.

### Como o projeto foi dividido

Para facilitar e não complicar a parte em C, dividi o trabalho em duas partes:

1. **Python (download.py)**: Serve só para a parte mais "chata" de lidar com o dataset original (que tem vários gigabytes). Ele baixa ou gera a amostra que você quiser, desduplica os produtos e já gera dois arquivos CSV limpos e ordenados dentro da pasta "data".
2. **C (main.c)**: É o núcleo do trabalho de AED II. Ele lê os CSVs, monta os arquivos binários (.bin) com registros de tamanho fixo terminados em "\n" e cria os índices parciais (.idx). Todas as buscas (binária e por índice), inserções e remoções são feitas aqui, direto em disco com "fseek".

---

### O que são os arquivos de dados?

Criamos dois arquivos binários dentro da pasta "data":

* **produtos.bin**: É basicamente o catálogo da loja. Guarda o ID único do produto, categoria, marca e preço. Ele fica ordenado pelo ID do produto (que é a chave primária).
* **acessos.bin**: É o histórico de eventos de quem navegou no site. Cada linha representa uma ação (se o usuário apenas visualizou o produto, se colocou no carrinho ou se comprou), além de guardar quem fez e a data. A chave aqui é um ID sequencial incremental.

Como exigido pelo trabalho, os campos de texto têm tamanho fixo completados com espaços em branco no final e cada registro encerra com um \n.

---

### Sobre o índice parcial e o INTERVAL

Em vez de criar um índice para cada registro (o que deixaria o arquivo de índice gigante), usamos um **índice parcial**. 

A cada INTERVAL = 10 registros no arquivo de dados, salvamos 1 entrada no arquivo de índice contendo o deslocamento (idx) e a chave daquele bloco. 

Na hora de consultar pelo índice:
1. O programa faz uma busca binária dentro do array de índices para achar qual bloco contém o ID que queremos.
2. Dá um fseek direto para o byte de início daquele bloco no arquivo binário.
3. Lê no máximo 10 registros sequenciais até achar a chave (ou interrompe se achar um ID maior).

O índice parcial é reconstruído automaticamente sempre que um registro é adicionado ou removido, e é salvo em disco (.bin) ao fechar o programa e recarregado ao abrir.

---

### Consultas de negócio (perguntas sobre o dataset)

No menu tem a opção 7, que responde algumas perguntas direto lendo os binários:
- **Taxa de conversão**: calcula quantas visualizações viraram compra de verdade (costuma ficar entre 2% e 5%).
- **Faixa de preços**: varre os produtos para achar o preço médio, o produto mais barato e o mais caro da loja.

---

### Dependências e como rodar sem instalar nada

O código em C só usa bibliotecas padrão, então não precisa instalar nada além do gcc (ou do make, que é bem útil).

Já no Python, se você optar por baixar o dataset oficial direto da fonte, ele usa a biblioteca kagglehub. Se não quiser instalar no seu computador, o programa faz o seguinte:

- O script sempre verifica primeiro se existe uma pasta chamada "dependencies" na raiz do projeto.
- Deixei um arquivo compactado (dependencies.rar) junto no repositório. Se você quiser rodar sem instalar absolutamente nada no seu PC, é só **extrair esse `.rar` na pasta do projeto**. O Python vai puxar tudo direto de lá.
- Caso prefira não extrair nada, aí só precisa se certificar de ter o kagglehub instalado no seu Python (pip install kagglehub).

---

### Como rodar

Basta compilar o C e executar. O próprio executável já roda primeiro o script em Python caso os arquivos ainda não tenham sido gerados:

```bash
# Se tiver make:
make
make run

# Ou compilar direto no gcc:
gcc main.c -o trabalho
./trabalho
```

Ao abrir, o terminal vai perguntar se você quer usar a amostra local rápida ou baixar o dataset original, e em seguida já abre o menu principal em C com todas as operações.
