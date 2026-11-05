// Testes da ArvoreAVL. Usa assert, validar() e std::map como oraculo.
#undef NDEBUG
#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "arvore_avl.hpp"

namespace {

int testes_passados = 0;

template <class F>
void executar(const char* nome, F teste) {
    teste();
    ++testes_passados;
    std::cout << "[ok] " << nome << '\n';
}

// Limite teorico da AVL: h < 1.4405 * log2(n + 2) - 0.3277.
bool altura_dentro_do_limite(int altura, std::size_t n) {
    return altura <= static_cast<int>(1.4405 * std::log2(static_cast<double>(n) + 2.0) - 0.3277);
}

template <class K, class V, class C>
std::vector<std::pair<K, V>> conteudo(const ArvoreAVL<K, V, C>& a) {
    std::vector<std::pair<K, V>> v;
    v.reserve(a.tamanho());
    a.em_ordem([&v](const K& k, const V& val) { v.emplace_back(k, val); });
    return v;
}

template <class K, class V, class C>
bool igual(const ArvoreAVL<K, V, C>& a, const std::map<K, V, C>& m) {
    return a.tamanho() == m.size() && conteudo(a) == std::vector<std::pair<K, V>>(m.begin(), m.end());
}

// Chave da raiz, obtida pela pre-ordem (profundidade 0).
int raiz_de(const ArvoreAVL<int, int>& a) {
    int raiz = -1;
    a.pre_ordem([&raiz](int k, int, int prof) {
        if (prof == 0) raiz = k;
    });
    return raiz;
}

void teste_vazia() {
    ArvoreAVL<int, int> a;
    assert(a.vazia() && a.tamanho() == 0 && a.altura() == 0);
    assert(a.buscar(1) == nullptr && !a.contem(1));
    assert(!a.remover(1));
    assert(a.minimo() == nullptr && a.maximo() == nullptr);
    assert(a.validar());
    assert(a.desenhar().empty());
    int visitas = 0;
    a.em_ordem([&visitas](int, int) { ++visitas; });
    assert(visitas == 0);
}

void teste_um_elemento() {
    ArvoreAVL<int, int> a;
    assert(a.inserir(5, 50));
    assert(a.tamanho() == 1 && a.altura() == 1 && a.validar());
    assert(*a.buscar(5) == 50);
    assert(*a.minimo() == 5 && *a.maximo() == 5);
    assert(a.remover(5));
    assert(a.vazia() && a.altura() == 0 && a.validar());
}

// Cada um dos 4 casos de desbalanceamento deve terminar com raiz = 2.
void teste_rotacoes() {
    const int casos[4][3] = {
        {3, 2, 1},  // esquerda-esquerda -> rotacao simples a direita
        {1, 2, 3},  // direita-direita   -> rotacao simples a esquerda
        {3, 1, 2},  // esquerda-direita  -> rotacao dupla
        {1, 3, 2},  // direita-esquerda  -> rotacao dupla
    };
    for (const auto& caso : casos) {
        ArvoreAVL<int, int> a;
        for (int k : caso) a.inserir(k, k);
        assert(a.altura() == 2);
        assert(raiz_de(a) == 2);
        assert(a.validar());
    }
}

void teste_duplicados_atualizam() {
    ArvoreAVL<int, std::string> a;
    assert(a.inserir(1, "a"));
    assert(!a.inserir(1, "b"));  // atualiza, nao duplica
    assert(a.tamanho() == 1 && *a.buscar(1) == "b");
    *a.buscar(1) = "c";  // acesso mutavel
    assert(*a.buscar(1) == "c");
    for (int i = 0; i < 100; ++i) a.inserir(i % 10, std::to_string(i));
    assert(a.tamanho() == 10 && a.validar());
    assert(*a.buscar(3) == "93");
}

void teste_remocao_casos() {
    ArvoreAVL<int, int> a;
    for (int k : {50, 30, 70, 20, 40, 60, 80, 10, 45, 65}) a.inserir(k, k);
    assert(a.validar());
    assert(a.remover(10));  // folha
    assert(a.validar());
    assert(a.remover(20));  // folha que restou apos a remocao anterior
    assert(a.validar());
    assert(a.remover(60));  // um filho
    assert(a.validar());
    assert(a.remover(50));  // raiz com dois filhos
    assert(a.validar());
    assert(!a.remover(50));
    assert((conteudo(a) == std::vector<std::pair<int, int>>{{30, 30}, {40, 40}, {45, 45}, {65, 65}, {70, 70}, {80, 80}}));
}

void teste_sequencial_balanceado() {
    constexpr int n = 100000;
    ArvoreAVL<int, int> crescente;
    ArvoreAVL<int, int> decrescente;
    for (int i = 0; i < n; ++i) {
        crescente.inserir(i, i);
        decrescente.inserir(n - i, i);
    }
    assert(crescente.validar() && decrescente.validar());
    assert(altura_dentro_do_limite(crescente.altura(), n));
    assert(altura_dentro_do_limite(decrescente.altura(), n));
    assert(crescente.altura() <= 17);  // log2(100000) ~ 16.6
    for (int i = 0; i < n; i += 2) assert(crescente.remover(i));
    assert(crescente.tamanho() == n / 2 && crescente.validar());
    assert(*crescente.minimo() == 1 && *crescente.maximo() == n - 1);
}

void teste_aleatorio_oraculo() {
    std::mt19937 gerador(2026);
    for (int rodada = 0; rodada < 10; ++rodada) {
        ArvoreAVL<int, int> a;
        std::map<int, int> m;
        const int faixa = rodada < 5 ? 50 : 5000;  // faixa pequena = muitos duplicados
        for (int passo = 0; passo < 20000; ++passo) {
            const int k = static_cast<int>(gerador() % faixa);
            const int v = static_cast<int>(gerador());
            switch (gerador() % 4) {
                case 0:
                case 1: {
                    const bool novo = m.find(k) == m.end();
                    m[k] = v;
                    assert(a.inserir(k, v) == novo);
                    break;
                }
                case 2:
                    assert(a.remover(k) == (m.erase(k) == 1));
                    break;
                default: {
                    auto it = m.find(k);
                    const int* p = a.buscar(k);
                    assert((p == nullptr) == (it == m.end()));
                    if (p) assert(*p == it->second);
                    break;
                }
            }
            assert(a.tamanho() == m.size());
            if (passo % 1000 == 0) assert(a.validar());
        }
        assert(a.validar());
        assert(igual(a, m));
        assert(altura_dentro_do_limite(a.altura(), a.tamanho()));
        if (!m.empty()) {
            assert(*a.minimo() == m.begin()->first);
            assert(*a.maximo() == m.rbegin()->first);
        }
    }
}

void teste_copia_movimento() {
    ArvoreAVL<std::string, int> a;
    for (int i = 0; i < 100; ++i) a.inserir("k" + std::to_string(i), i);
    ArvoreAVL<std::string, int> b = a;  // copia profunda
    b.remover("k0");
    b.inserir("novo", -1);
    assert(a.tamanho() == 100 && a.contem("k0") && !a.contem("novo"));
    assert(b.tamanho() == 100 && !b.contem("k0") && b.validar());

    ArvoreAVL<std::string, int> c = std::move(b);
    assert(b.vazia() && b.altura() == 0 && b.validar());
    assert(c.tamanho() == 100 && c.validar());
    b.inserir("reuso", 1);  // objeto movido continua utilizavel
    assert(b.tamanho() == 1);

    a = c;
    assert(conteudo(a) == conteudo(c));
    const auto& mesmo = a;
    a = mesmo;  // auto-atribuicao
    assert(a.tamanho() == 100 && a.validar());
    c = std::move(a);
    assert(a.vazia() && c.tamanho() == 100);

    static_assert(std::is_nothrow_move_constructible_v<ArvoreAVL<std::string, int>>);
    static_assert(std::is_nothrow_move_assignable_v<ArvoreAVL<std::string, int>>);
}

void teste_comparador_e_valor_movel() {
    ArvoreAVL<int, std::unique_ptr<int>, std::greater<int>> a;  // ordem decrescente
    for (int i = 0; i < 10; ++i) a.inserir(i, std::make_unique<int>(i * i));
    std::vector<int> chaves;
    a.em_ordem([&chaves](int k, const std::unique_ptr<int>& v) {
        assert(*v == k * k);
        chaves.push_back(k);
    });
    assert((chaves == std::vector<int>{9, 8, 7, 6, 5, 4, 3, 2, 1, 0}));
    assert(a.validar());
    assert(*a.minimo() == 9);  // "minimo" segundo o comparador
}

void teste_limpar() {
    ArvoreAVL<int, int> a;
    for (int i = 0; i < 1000; ++i) a.inserir(i, i);
    a.limpar();
    assert(a.vazia() && a.altura() == 0 && a.validar());
    a.inserir(1, 1);
    assert(a.tamanho() == 1);
}

}  // namespace

int main() {
    executar("arvore vazia", teste_vazia);
    executar("um elemento", teste_um_elemento);
    executar("quatro casos de rotacao", teste_rotacoes);
    executar("chave duplicada atualiza valor", teste_duplicados_atualizam);
    executar("remocao: folha, um filho, dois filhos", teste_remocao_casos);
    executar("insercao sequencial permanece balanceada", teste_sequencial_balanceado);
    executar("aleatorio vs std::map", teste_aleatorio_oraculo);
    executar("copia e movimento", teste_copia_movimento);
    executar("comparador customizado e valor somente movel", teste_comparador_e_valor_movel);
    executar("limpar", teste_limpar);
    std::cout << testes_passados << " testes passaram\n";
    return 0;
}
