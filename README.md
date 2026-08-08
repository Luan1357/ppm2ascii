# Conversor PPM para ASCII Art

## Descrição do projeto

Programa em C que converte imagens no formato **PPM binário (P6)** em arte ASCII, exibindo o resultado no terminal e salvando-o em um arquivo `.txt` com o mesmo nome base da imagem.

O **parsing do cabeçalho é feito manualmente**, sem bibliotecas externas: o programa valida o magic number `P6`, ignora comentários (linhas iniciadas com `#`) em qualquer posição do cabeçalho, lê largura e altura, e trata o `maxval` tanto em **8 bits** (3 bytes por pixel) quanto em **16 bits por canal** (6 bytes por pixel, big-endian, normalizados para 8 bits). A especificação NetPBM, que exige exatamente um caractere de espaço em branco separando o cabeçalho dos dados binários, também é validada.

A conversão de cada pixel para caractere usa o cálculo de **luminância pelo padrão ITU-R BT.601**:

```
L = 0.299 × R + 0.587 × G + 0.114 × B
```

O valor de luminância resultante é mapeado proporcionalmente para uma **rampa de 57 caracteres** de intensidade, ordenada do mais escuro (espaço) ao mais claro (`$`).

## Limitação técnica

- **Sem correção de aspect ratio:** cada pixel é mapeado 1:1 para um caractere. Como caracteres de terminal são mais altos do que largos, a imagem resultante aparece verticalmente esticada.
- **Sem redimensionamento automático:** a saída tem exatamente as dimensões da imagem de entrada. Recomenda-se usar imagens pequenas ou pré-reduzidas para que o resultado caiba no terminal.

## Como usar

```
gcc -Wall -Wextra -o ppm2ascii ppm2ascii.c
./ppm2ascii
```

O programa solicita o nome do arquivo de imagem e gera um `.txt` com o mesmo nome base (ex.: `foto.ppm` → `foto.txt`).

## Disciplina

Programação Imperativa — PUCPR, 2026

## Tecnologias utilizadas

- C

## Estrutura do repositório

- `ppm2ascii.c` — código-fonte do conversor.

## Contexto acadêmico

Projeto desenvolvido por **Luan Alfredo Lugarini**, estudante de Engenharia da Computação na PUCPR.
