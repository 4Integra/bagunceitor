# Bagunceitor

Projeto em C para aplicar distorções **reversíveis** a imagens RGB, preservando a possibilidade de recuperar com exatidão os valores dos pixels.

Vídeo demonstração: https://youtu.be/rArd_4Mh7iQ

## Efeitos

- **Cor:** Efeito de posterização neon. XOR no canal vermelho, soma modular no verde e troca dos canais vermelho e azul.
- **Espiral:** deslocamento circular de blocos de pixels (32 × 32) em percurso espiral. Blocos incompletos nas bordas não são movimentados.
- **Ambos:** aplica cor e, em seguida, espiral; a reversão ocorre na ordem inversa.

## Compilação e execução

Compile com o Makefile do projeto:

```bash
make
```

Execute com:

```bash
./bagunceitor imagem.png [aplicar] [reverter]
```

Os modos disponíveis são `nenhum`, `cor`, `espiral` e `ambos`. Exemplos:

```bash
./bagunceitor imagem.png cor cor
./bagunceitor imagem.png espiral espiral
./bagunceitor imagem.png ambos ambos
./bagunceitor imagem.png ambos nenhum
```

No Windows, o executável pode ser `bagunceitor.exe`.

## Saídas e verificação

- `saida_baguncada.png`: resultado das transformações escolhidas.
- `saida_restaurada.png`: resultado após as operações inversas selecionadas.
- `memcmp()`: verifica se os **bytes RGB dos pixels em memória** são idênticos aos da imagem carregada.

Para obter a restauração completa, escolha o mesmo conjunto de efeitos em `[aplicar]` e `[reverter]`. A comparação não garante que os bytes do *arquivo PNG* original (metadados ou compressão) sejam iguais. A imagem é carregada como RGB, portanto um eventual canal alfa não é preservado.

## Declaração de uso de IA

Este relatório (README) foi gerado pelo **ChatGPT, modelo GPT-6**. Houve também auxílio do ChatGPT na criação e revisão dos algoritmos, especialmente na formulação das operações inversas e na verificação de sua precisão matemática e reversibilidade.
