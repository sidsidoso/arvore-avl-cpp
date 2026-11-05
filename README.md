# Árvore AVL em C++

[![CI](https://github.com/sidsidoso/arvore-avl-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/sidsidoso/arvore-avl-cpp/actions/workflows/ci.yml)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
[![Licenca: MIT](https://img.shields.io/badge/licen%C3%A7a-MIT-green.svg)](LICENSE)

Implementação header-only de uma **árvore AVL genérica** (`ArvoreAVL<Chave, Valor, Comparador>`) em C++17, funcionando como um mapa ordenado chave → valor. Os filhos são `std::unique_ptr`, as rotações simples e duplas mantêm a árvore balanceada após cada inserção e remoção, e uma função `validar()` verifica os invariantes (ordem e balanceamento) nos testes. Projeto da disciplina de Estrutura de Dados.

## Conceito

Uma AVL é uma árvore binária de busca em que, para **todo nó**, a diferença entre as alturas das subárvores esquerda e direita (o *fator de balanceamento*) é -1, 0 ou +1. Isso garante altura `h < 1,44 · log2(n + 2)`, logo busca, inserção e remoção em O(log n), mesmo para entradas ordenadas (que degeneram uma BST comum em lista).

Depois de inserir ou remover, o caminho de volta até a raiz é recalculado; se algum nó ficar com fator ±2, aplica-se uma das quatro correções:

**1. Esquerda-Esquerda (rotação simples à direita)**

```
          z                      y
         / \                   /   \
        y   D                 x     z
       / \        ==>        / \   / \
      x   C                 A   B C   D
     / \
    A   B
```

**2. Direita-Direita (rotação simples à esquerda)**

```
      z                          y
     / \                       /   \
    A   y                     z     x
       / \        ==>        / \   / \
      B   x                 A   B C   D
         / \
        C   D
```

**3. Esquerda-Direita (rotação dupla: esquerda em `y`, depois direita em `z`)**

```
        z                z                  x
       / \              / \               /   \
      y   D            x   D             y     z
     / \       ==>    / \      ==>      / \   / \
    A   x            y   C             A   B C   D
       / \          / \
      B   C        A   B
```

**4. Direita-Esquerda (rotação dupla: direita em `y`, depois esquerda em `z`)**

```
      z                z                    x
     / \              / \                 /   \
    A   y            A   x               z     y
       / \     ==>      / \      ==>    / \   / \
      x   D            B   y           A   B C   D
     / \                  / \
    B   C                C   D
```

Cada nó armazena a **altura** da própria subárvore, então o fator de balanceamento é calculado em O(1).

## Complexidade

| Operação                    | Melhor   | Médio    | Pior     | Observação                                    |
|-----------------------------|----------|----------|----------|-----------------------------------------------|
| `buscar` / `contem`         | O(1)     | O(log n) | O(log n) | iterativo, sem recursão                       |
| `inserir`                   | O(log n) | O(log n) | O(log n) | no máximo 1 rotação (simples ou dupla)        |
| `remover`                   | O(log n) | O(log n) | O(log n) | até O(log n) rotações no caminho de volta     |
| `minimo` / `maximo`         | O(log n) | O(log n) | O(log n) |                                               |
| `altura` / `tamanho`        | O(1)     | O(1)     | O(1)     | valores mantidos na estrutura                 |
| `em_ordem` / `pre_ordem`    | O(n)     | O(n)     | O(n)     | visitante (callback/lambda)                   |
| `validar`                   | O(n)     | O(n)     | O(n)     | usado nos testes                              |
| cópia                       | O(n)     | O(n)     | O(n)     | cópia profunda que preserva a forma           |
| movimento / `trocar`        | O(1)     | O(1)     | O(1)     | `noexcept`                                    |
| **Espaço**                  | O(n)     | O(n)     | O(n)     | altura (int) + 2 ponteiros por nó; pilha O(log n) |

## Decisões de implementação e otimizações

- **`std::unique_ptr` para os filhos**: posse explícita, nenhum `delete` manual e destruição automática. Como a altura é O(log n), a destruição recursiva dos `unique_ptr` não estoura a pilha.
- **Rotações por referência a `unique_ptr&`**: a função recebe o "slot" que aponta para a subárvore (a raiz ou o campo `esq`/`dir` do pai) e religa os ponteiros ali mesmo, sem ponteiro para o pai.
- **Remoção sem copiar chave/valor**: no caso de dois filhos, o nó sucessor (mínimo da subárvore direita) é *desencaixado* e religado no lugar do removido, trocando apenas ponteiros. Funciona com tipos caros de copiar ou somente-móveis (ex.: `std::unique_ptr` como valor).
- **`inserir(Chave, Valor)` por valor + `std::move`**: o par é movido até o nó novo, sem cópias extras. Se a chave já existir, o valor é atualizado (como `std::map::insert_or_assign`) e nenhuma rotação é feita.
- **Inserção sobe parando cedo**: se a chave já existia, a volta da recursão não rebalanceia nada.
- **`buscar` iterativo** retorna `Valor*` (ou `nullptr`), permitindo ler e alterar o valor sem busca dupla.
- **Comparador como parâmetro de template** (padrão `std::less<Chave>`), como nos containers da STL; só `<` é usado, nunca `==`.
- **`validar()`** confere, em uma única passada: ordem estrita (com limites inferior/superior herdados, não só pai-filho), altura armazenada correta, fator em [-1, 1] e contagem de nós igual a `tamanho()`.
- Cópia profunda com `clonar` (copia-e-troca na atribuição para garantia forte); movimento `noexcept` deixando a origem vazia e válida.

## Estrutura de pastas

```
arvore-avl-cpp/
├── include/
│   └── arvore_avl.hpp         # implementação (header-only)
├── src/
│   └── main.cpp               # demonstração
├── tests/
│   └── test_arvore_avl.cpp    # testes (assert + validar() + std::map como oráculo)
├── .github/workflows/ci.yml   # CI: testes com sanitizers + demo
├── Makefile
├── LICENSE
└── README.md
```

## Como compilar, executar e testar

### Linux / macOS (make)

```bash
make            # compila demo e testes em build/
make run        # executa a demonstração
make test       # executa os testes
make test SANITIZE=1   # testes com AddressSanitizer + UndefinedBehaviorSanitizer
make clean
```

### Windows (g++ do MinGW-w64 / WinLibs)

```bat
mkdir build
g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude src/main.cpp -o build/demo.exe
g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude tests/test_arvore_avl.cpp -o build/testes.exe
build\demo.exe
build\testes.exe
```

### Uso

```cpp
#include "arvore_avl.hpp"

ArvoreAVL<std::string, int> idades;
idades.inserir("ana", 20);
idades.inserir("bruno", 31);
if (int* v = idades.buscar("ana")) ++*v;
idades.remover("bruno");
idades.em_ordem([](const std::string& k, int v) { std::cout << k << '=' << v << '\n'; });
```

## Exemplo de saída

`make run` (a árvore é desenhada "deitada": raiz na coluna da esquerda, filho direito acima e esquerdo abaixo):

```
Insercao de 1..7 em ordem crescente
tamanho: 7, altura: 3 (uma BST sem balanceamento teria altura 7)
arvore (deitada, raiz a esquerda):
        7
    6
        5
4
        3
    2
        1

Rotacao dupla (insercao 10, 30, 20):
    30
20
    10

buscar(5) -> valor 5
contem(42)? false
apos atualizar, buscar(5) -> cinco

Apos remover 4, 1 e 2:
        7
    6
5
    3
altura: 3, invariantes ok? true

Em ordem: 3="valor 3" 5="cinco" 6="valor 6" 7="valor 7"
Frequencia: maca:1 pera:2 uva:3
```

`make test`:

```
[ok] arvore vazia
[ok] um elemento
[ok] quatro casos de rotacao
[ok] chave duplicada atualiza valor
[ok] remocao: folha, um filho, dois filhos
[ok] insercao sequencial permanece balanceada
[ok] aleatorio vs std::map
[ok] copia e movimento
[ok] comparador customizado e valor somente movel
[ok] limpar
10 testes passaram
```

Os testes cobrem árvore vazia, um elemento, os quatro casos de rotação, chaves duplicadas, todos os casos de remoção, 100 mil inserções sequenciais (crescentes e decrescentes) e 200 mil operações aleatórias comparadas com `std::map` como oráculo, checando `validar()` e o limite teórico de altura ao longo do caminho.

## Referências

- ADELSON-VELSKY, G. M.; LANDIS, E. M. An algorithm for the organization of information. *Soviet Mathematics Doklady*, v. 3, p. 1259-1263, 1962.
- KNUTH, D. E. *The Art of Computer Programming, Vol. 3: Sorting and Searching*. 2. ed. Addison-Wesley, 1998. Seção 6.2.3 (árvores balanceadas).
- SZWARCFITER, J. L.; MARKENZON, L. *Estruturas de Dados e Seus Algoritmos*. 3. ed. LTC, 2010. Cap. 5.
- CORMEN, T. H. et al. *Algoritmos: Teoria e Prática*. 3. ed. Elsevier, 2012. Problema 13-3 (árvores AVL).
- cppreference: [std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr), [std::map](https://en.cppreference.com/w/cpp/container/map).

## Licença

Distribuído sob a licença MIT. Veja [LICENSE](LICENSE).
