// Demonstracao da ArvoreAVL.
#include <iostream>
#include <string>

#include "arvore_avl.hpp"

int main() {
    ArvoreAVL<int, std::string> arvore;

    // Insercao em ordem crescente: numa BST comum viraria uma lista (altura 7).
    for (int i = 1; i <= 7; ++i) arvore.inserir(i, "valor " + std::to_string(i));
    std::cout << "Insercao de 1..7 em ordem crescente\n";
    std::cout << "tamanho: " << arvore.tamanho() << ", altura: " << arvore.altura()
              << " (uma BST sem balanceamento teria altura 7)\n";
    std::cout << "arvore (deitada, raiz a esquerda):\n" << arvore.desenhar() << '\n';

    // Rotacao dupla: 10, 30, 20 forca o caso direita-esquerda.
    ArvoreAVL<int, int> dupla;
    dupla.inserir(10, 0);
    dupla.inserir(30, 0);
    dupla.inserir(20, 0);
    std::cout << "Rotacao dupla (insercao 10, 30, 20):\n" << dupla.desenhar() << '\n';

    // Busca e atualizacao.
    if (const std::string* v = arvore.buscar(5)) std::cout << "buscar(5) -> " << *v << '\n';
    std::cout << "contem(42)? " << std::boolalpha << arvore.contem(42) << '\n';
    arvore.inserir(5, "cinco");  // chave existente: atualiza o valor
    std::cout << "apos atualizar, buscar(5) -> " << *arvore.buscar(5) << "\n\n";

    // Remocao com rebalanceamento.
    arvore.remover(4);  // raiz, com dois filhos
    arvore.remover(1);
    arvore.remover(2);
    std::cout << "Apos remover 4, 1 e 2:\n" << arvore.desenhar();
    std::cout << "altura: " << arvore.altura() << ", invariantes ok? " << arvore.validar() << "\n\n";

    // Percurso em ordem com callback.
    std::cout << "Em ordem:";
    arvore.em_ordem([](int chave, const std::string& valor) {
        std::cout << ' ' << chave << '=' << '"' << valor << '"';
    });
    std::cout << '\n';

    // Mapa de palavras -> contagem com chave std::string.
    ArvoreAVL<std::string, int> frequencia;
    for (const char* palavra : {"pera", "uva", "maca", "uva", "pera", "uva"}) {
        int* atual = frequencia.buscar(palavra);
        if (atual)
            ++*atual;
        else
            frequencia.inserir(palavra, 1);
    }
    std::cout << "Frequencia:";
    frequencia.em_ordem([](const std::string& p, int n) { std::cout << ' ' << p << ':' << n; });
    std::cout << '\n';
    return 0;
}
