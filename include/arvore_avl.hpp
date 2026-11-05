// arvore_avl.hpp - arvore AVL generica (mapa ordenado chave -> valor).
//
// Cada no guarda a altura da sua subarvore. Depois de toda insercao/remocao o
// caminho de volta ate a raiz e rebalanceado com rotacoes simples ou duplas,
// garantindo |altura(esq) - altura(dir)| <= 1 em todos os nos.
// Os filhos sao std::unique_ptr: a posse e explicita e nao ha delete manual.
#ifndef ARVORE_AVL_HPP
#define ARVORE_AVL_HPP

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

template <class Chave, class Valor, class Comparador = std::less<Chave>>
class ArvoreAVL {
    struct No {
        Chave chave;
        Valor valor;
        int altura = 1;  // folha tem altura 1; ponteiro nulo tem altura 0
        std::unique_ptr<No> esq;
        std::unique_ptr<No> dir;

        No(Chave c, Valor v) : chave(std::move(c)), valor(std::move(v)) {}
    };
    using Ptr = std::unique_ptr<No>;

public:
    using size_type = std::size_t;

    // ---------- construcao, copia, movimento ----------

    ArvoreAVL() = default;
    explicit ArvoreAVL(Comparador comp) : comp_(std::move(comp)) {}

    ArvoreAVL(const ArvoreAVL& outra)
        : raiz_(clonar(outra.raiz_.get())), tamanho_(outra.tamanho_), comp_(outra.comp_) {}

    ArvoreAVL(ArvoreAVL&& outra) noexcept
        : raiz_(std::move(outra.raiz_)),
          tamanho_(std::exchange(outra.tamanho_, 0)),
          comp_(std::move(outra.comp_)) {}

    ArvoreAVL& operator=(const ArvoreAVL& outra) {
        if (this != &outra) {
            ArvoreAVL temp(outra);  // copia-e-troca: garantia forte
            trocar(temp);
        }
        return *this;
    }

    ArvoreAVL& operator=(ArvoreAVL&& outra) noexcept {
        if (this != &outra) {
            raiz_ = std::move(outra.raiz_);
            tamanho_ = std::exchange(outra.tamanho_, 0);
            comp_ = std::move(outra.comp_);
        }
        return *this;
    }

    ~ArvoreAVL() = default;  // profundidade O(log n): a destruicao recursiva e segura

    void trocar(ArvoreAVL& outra) noexcept {
        using std::swap;
        swap(raiz_, outra.raiz_);
        swap(tamanho_, outra.tamanho_);
        swap(comp_, outra.comp_);
    }

    // ---------- consulta ----------

    [[nodiscard]] size_type tamanho() const noexcept { return tamanho_; }
    [[nodiscard]] bool vazia() const noexcept { return tamanho_ == 0; }
    [[nodiscard]] int altura() const noexcept { return altura_de(raiz_); }

    // Retorna ponteiro para o valor ou nullptr se a chave nao existir. O(log n).
    Valor* buscar(const Chave& chave) noexcept {
        return const_cast<Valor*>(std::as_const(*this).buscar(chave));
    }
    const Valor* buscar(const Chave& chave) const noexcept {
        const No* atual = raiz_.get();
        while (atual) {
            if (comp_(chave, atual->chave))
                atual = atual->esq.get();
            else if (comp_(atual->chave, chave))
                atual = atual->dir.get();
            else
                return &atual->valor;
        }
        return nullptr;
    }

    [[nodiscard]] bool contem(const Chave& chave) const noexcept { return buscar(chave) != nullptr; }

    // Menor/maior chave (nullptr se vazia).
    const Chave* minimo() const noexcept {
        const No* n = raiz_.get();
        if (!n) return nullptr;
        while (n->esq) n = n->esq.get();
        return &n->chave;
    }
    const Chave* maximo() const noexcept {
        const No* n = raiz_.get();
        if (!n) return nullptr;
        while (n->dir) n = n->dir.get();
        return &n->chave;
    }

    // ---------- modificacao ----------

    // Insere o par. Se a chave ja existir, atualiza o valor e retorna false.
    bool inserir(Chave chave, Valor valor) {
        bool inseriu = inserir_rec(raiz_, chave, valor);
        if (inseriu) ++tamanho_;
        return inseriu;
    }

    // Remove a chave. Retorna false se ela nao existir.
    bool remover(const Chave& chave) {
        bool removeu = remover_rec(raiz_, chave);
        if (removeu) --tamanho_;
        return removeu;
    }

    void limpar() noexcept {
        raiz_.reset();
        tamanho_ = 0;
    }

    // ---------- percursos ----------

    // Visita os pares em ordem crescente de chave: visitante(chave, valor).
    template <class Visitante>
    void em_ordem(Visitante&& visitante) const {
        em_ordem_rec(raiz_.get(), visitante);
    }

    // Visita em pre-ordem informando a profundidade: visitante(chave, valor, prof).
    template <class Visitante>
    void pre_ordem(Visitante&& visitante) const {
        pre_ordem_rec(raiz_.get(), visitante, 0);
    }

    // Desenho da arvore deitada: raiz na coluna 0, filho direito acima, esquerdo abaixo.
    // Exige operator<< para Chave.
    std::string desenhar() const {
        std::ostringstream saida;
        desenhar_rec(saida, raiz_.get(), 0);
        return saida.str();
    }

    // ---------- invariantes ----------

    // Verifica: ordem de busca estrita, alturas armazenadas corretas,
    // fator de balanceamento em [-1, 1] em todo no e contagem == tamanho().
    [[nodiscard]] bool validar() const {
        size_type contagem = 0;
        return validar_rec(raiz_.get(), nullptr, nullptr, contagem) >= 0 && contagem == tamanho_;
    }

private:
    Ptr raiz_;
    size_type tamanho_ = 0;
    Comparador comp_{};

    static int altura_de(const Ptr& n) noexcept { return n ? n->altura : 0; }

    static void atualizar(No& n) noexcept {
        n.altura = 1 + std::max(altura_de(n.esq), altura_de(n.dir));
    }

    static int fator(const No& n) noexcept { return altura_de(n.esq) - altura_de(n.dir); }

    /*
     *        r                e
     *       / \              / \
     *      e   C    ==>     A   r
     *     / \                  / \
     *    A   B                B   C
     */
    static void rotacionar_direita(Ptr& r) noexcept {
        Ptr e = std::move(r->esq);
        r->esq = std::move(e->dir);
        atualizar(*r);
        e->dir = std::move(r);
        atualizar(*e);
        r = std::move(e);
    }

    // Espelho da rotacao a direita.
    static void rotacionar_esquerda(Ptr& r) noexcept {
        Ptr d = std::move(r->dir);
        r->dir = std::move(d->esq);
        atualizar(*r);
        d->esq = std::move(r);
        atualizar(*d);
        r = std::move(d);
    }

    // Recalcula a altura e aplica a rotacao necessaria (no maximo duas).
    static void balancear(Ptr& n) noexcept {
        atualizar(*n);
        const int fb = fator(*n);
        if (fb > 1) {
            if (fator(*n->esq) < 0) rotacionar_esquerda(n->esq);  // caso esquerda-direita
            rotacionar_direita(n);                                // caso esquerda-esquerda
        } else if (fb < -1) {
            if (fator(*n->dir) > 0) rotacionar_direita(n->dir);   // caso direita-esquerda
            rotacionar_esquerda(n);                               // caso direita-direita
        }
    }

    bool inserir_rec(Ptr& n, Chave& chave, Valor& valor) {
        if (!n) {
            n = std::make_unique<No>(std::move(chave), std::move(valor));
            return true;
        }
        bool inseriu;
        if (comp_(chave, n->chave)) {
            inseriu = inserir_rec(n->esq, chave, valor);
        } else if (comp_(n->chave, chave)) {
            inseriu = inserir_rec(n->dir, chave, valor);
        } else {
            n->valor = std::move(valor);
            return false;  // estrutura inalterada: nada a rebalancear
        }
        if (inseriu) balancear(n);
        return inseriu;
    }

    // Desencaixa o menor no da subarvore (rebalanceando na volta) e o devolve.
    static Ptr extrair_minimo(Ptr& n) noexcept {
        if (!n->esq) {
            Ptr minimo = std::move(n);
            n = std::move(minimo->dir);
            return minimo;
        }
        Ptr minimo = extrair_minimo(n->esq);
        balancear(n);
        return minimo;
    }

    bool remover_rec(Ptr& n, const Chave& chave) {
        if (!n) return false;
        bool removeu;
        if (comp_(chave, n->chave)) {
            removeu = remover_rec(n->esq, chave);
        } else if (comp_(n->chave, chave)) {
            removeu = remover_rec(n->dir, chave);
        } else {
            if (!n->esq || !n->dir) {
                // zero ou um filho: o filho (ou nullptr) ocupa o lugar do no
                n = std::move(n->esq ? n->esq : n->dir);
            } else {
                // dois filhos: o sucessor (menor da direita) e religado no lugar;
                // nenhum par chave/valor e copiado ou movido, so ponteiros.
                Ptr sucessor = extrair_minimo(n->dir);
                sucessor->esq = std::move(n->esq);
                sucessor->dir = std::move(n->dir);
                n = std::move(sucessor);
            }
            removeu = true;
        }
        if (removeu && n) balancear(n);
        return removeu;
    }

    template <class Visitante>
    static void em_ordem_rec(const No* n, Visitante& visitante) {
        if (!n) return;
        em_ordem_rec(n->esq.get(), visitante);
        visitante(static_cast<const Chave&>(n->chave), static_cast<const Valor&>(n->valor));
        em_ordem_rec(n->dir.get(), visitante);
    }

    template <class Visitante>
    static void pre_ordem_rec(const No* n, Visitante& visitante, int profundidade) {
        if (!n) return;
        visitante(static_cast<const Chave&>(n->chave), static_cast<const Valor&>(n->valor),
                  profundidade);
        pre_ordem_rec(n->esq.get(), visitante, profundidade + 1);
        pre_ordem_rec(n->dir.get(), visitante, profundidade + 1);
    }

    static void desenhar_rec(std::ostringstream& saida, const No* n, int nivel) {
        if (!n) return;
        desenhar_rec(saida, n->dir.get(), nivel + 1);
        saida << std::string(static_cast<std::size_t>(nivel) * 4, ' ') << n->chave << '\n';
        desenhar_rec(saida, n->esq.get(), nivel + 1);
    }

    static Ptr clonar(const No* n) {
        if (!n) return nullptr;
        auto copia = std::make_unique<No>(n->chave, n->valor);
        copia->altura = n->altura;
        copia->esq = clonar(n->esq.get());
        copia->dir = clonar(n->dir.get());
        return copia;
    }

    // Retorna a altura da subarvore ou -1 se algum invariante for violado.
    int validar_rec(const No* n, const Chave* menor, const Chave* maior,
                    size_type& contagem) const {
        if (!n) return 0;
        if (menor && !comp_(*menor, n->chave)) return -1;
        if (maior && !comp_(n->chave, *maior)) return -1;
        const int he = validar_rec(n->esq.get(), menor, &n->chave, contagem);
        if (he < 0) return -1;
        const int hd = validar_rec(n->dir.get(), &n->chave, maior, contagem);
        if (hd < 0) return -1;
        if (he - hd > 1 || hd - he > 1) return -1;
        if (n->altura != 1 + std::max(he, hd)) return -1;
        ++contagem;
        return n->altura;
    }
};

#endif  // ARVORE_AVL_HPP
