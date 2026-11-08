// Exercice 4 : foncteur générique de tri avec contraintes C++20 (concepts)
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <vector>

// ----------------------------------------------------------------------
// Concept : un type est triable s'il supporte a < b convertible en bool
// ----------------------------------------------------------------------
template <typename T>
concept Sortable = requires(T a, T b) {
    { a < b } -> std::convertible_to<bool>;
};

// ----------------------------------------------------------------------
// Foncteurs de comparaison : operator() est un template contraint par Sortable
// ----------------------------------------------------------------------
struct Asc {
    template <Sortable T>
    bool operator()(const T& a, const T& b) const { return a < b; }
};

struct Desc {
    template <Sortable T>
    bool operator()(const T& a, const T& b) const { return b < a; }
};

// ----------------------------------------------------------------------
// Quicksort (partition de Hoare, pivot central).
// On récurse sur la plus petite partie et on boucle sur la plus grande :
// la profondeur de récursion reste en O(log n).
// ----------------------------------------------------------------------
namespace detail {
template <typename T, typename Comp>
void quickSortImpl(std::vector<T>& v, std::ptrdiff_t lo, std::ptrdiff_t hi, Comp& comp) {
    while (lo < hi) {
        T pivot = v[static_cast<std::size_t>(lo + (hi - lo) / 2)];
        std::ptrdiff_t i = lo;
        std::ptrdiff_t j = hi;

        while (i <= j) {
            while (comp(v[static_cast<std::size_t>(i)], pivot)) ++i;
            while (comp(pivot, v[static_cast<std::size_t>(j)])) --j;
            if (i <= j) {
                std::swap(v[static_cast<std::size_t>(i)], v[static_cast<std::size_t>(j)]);
                ++i;
                --j;
            }
        }

        if (j - lo < hi - i) {
            quickSortImpl(v, lo, j, comp);
            lo = i;
        } else {
            quickSortImpl(v, i, hi, comp);
            hi = j;
        }
    }
}
}  // namespace detail

template <Sortable T, typename Comp = std::less<T>>
    requires std::predicate<Comp&, const T&, const T&>
void quickSort(std::vector<T>& v, Comp comp = Comp{}) {
    if (v.size() < 2) return;
    detail::quickSortImpl(v, 0, static_cast<std::ptrdiff_t>(v.size()) - 1, comp);
}

// ======================================================================
// Tests
// ======================================================================

// Structure utilisateur avec operator<
struct Employe {
    std::string nom;
    int salaire;
    bool operator<(const Employe& o) const { return salaire < o.salaire; }
};

// Type sans operator< : doit être rejeté par le concept
struct NonTriable { int x; };

static_assert(Sortable<int>);
static_assert(Sortable<std::string>);
static_assert(Sortable<Employe>);
static_assert(!Sortable<NonTriable>, "NonTriable ne doit pas satisfaire Sortable");

int echecs = 0;

void verifier(const std::string& nom, bool ok) {
    std::cout << (ok ? "[OK]     " : "[ECHEC]  ") << nom << "\n";
    if (!ok) ++echecs;
}

template <typename T>
void afficher(const std::string& titre, const std::vector<T>& v) {
    std::cout << "  " << titre << " : ";
    for (const auto& e : v) std::cout << e << " ";
    std::cout << "\n";
}

int main() {
    // --- int ---
    std::cout << "=== std::vector<int> ===\n";
    std::vector<int> nombres = {42, 7, 19, 3, 88, 7, 0, -5, 23};
    afficher("Avant        ", nombres);

    quickSort(nombres);                          // comparateur par défaut : std::less
    afficher("Defaut (<)   ", nombres);
    verifier("int trie par défaut", std::is_sorted(nombres.begin(), nombres.end()));

    quickSort(nombres, Desc{});
    afficher("Desc         ", nombres);
    verifier("int trie decroissant",
             std::is_sorted(nombres.begin(), nombres.end(), std::greater<int>{}));

    quickSort(nombres, Asc{});
    verifier("int trie croissant (Asc)", std::is_sorted(nombres.begin(), nombres.end()));

    // --- std::string ---
    std::cout << "\n=== std::vector<std::string> ===\n";
    std::vector<std::string> mots = {"pomme", "banane", "cerise", "abricot", "kiwi", "figue"};
    afficher("Avant        ", mots);
    quickSort(mots, Asc{});
    afficher("Asc          ", mots);
    verifier("string trie croissant", std::is_sorted(mots.begin(), mots.end()));
    quickSort(mots, Desc{});
    afficher("Desc         ", mots);
    verifier("string trie decroissant",
             std::is_sorted(mots.begin(), mots.end(), std::greater<std::string>{}));

    // --- structure utilisateur ---
    std::cout << "\n=== std::vector<Employe> ===\n";
    std::vector<Employe> equipe = {
        {"Alice", 5200}, {"Bob", 3100}, {"Charlie", 4500}, {"Diane", 3900}
    };
    auto afficherEquipe = [&](const std::string& titre) {
        std::cout << "  " << titre << " : ";
        for (const auto& e : equipe) std::cout << e.nom << "(" << e.salaire << ") ";
        std::cout << "\n";
    };

    quickSort(equipe);                           // utilise Employe::operator<
    afficherEquipe("Par salaire croissant ");
    verifier("Employe trie par salaire",
             std::is_sorted(equipe.begin(), equipe.end()));

    quickSort(equipe, Desc{});
    afficherEquipe("Par salaire decroissant");
    verifier("Employe trie par salaire decroissant",
             std::is_sorted(equipe.begin(), equipe.end(), Desc{}));

    // Un comparateur arbitraire (lambda) est aussi accepté
    auto parNom = [](const Employe& a, const Employe& b) { return a.nom < b.nom; };
    quickSort(equipe, parNom);
    afficherEquipe("Par nom (lambda)      ");
    verifier("Employe trie par nom", std::is_sorted(equipe.begin(), equipe.end(), parNom));

    // --- cas limites ---
    std::cout << "\n=== Cas limites ===\n";
    std::vector<int> vide;
    quickSort(vide);
    verifier("vecteur vide", vide.empty());

    std::vector<int> un = {1};
    quickSort(un);
    verifier("un seul element", un.size() == 1 && un[0] == 1);

    std::vector<int> doublons(50, 7);
    quickSort(doublons);
    verifier("tous elements egaux", std::is_sorted(doublons.begin(), doublons.end()));

    std::vector<int> trie(10000);
    for (int i = 0; i < 10000; ++i) trie[static_cast<std::size_t>(i)] = i;
    quickSort(trie);
    verifier("deja trie (10 000 elements)", std::is_sorted(trie.begin(), trie.end()));

    std::vector<int> inverse(10000);
    for (int i = 0; i < 10000; ++i) inverse[static_cast<std::size_t>(i)] = 10000 - i;
    quickSort(inverse);
    verifier("trie a l'envers (10 000 elements)", std::is_sorted(inverse.begin(), inverse.end()));

    // --- test aléatoire volumineux ---
    std::cout << "\n=== Test aleatoire ===\n";
    std::mt19937 gen(12345);
    std::uniform_int_distribution<int> dist(-100000, 100000);
    std::vector<int> grand(200000);
    for (auto& e : grand) e = dist(gen);
    std::vector<int> reference = grand;

    quickSort(grand);
    std::sort(reference.begin(), reference.end());
    verifier("200 000 entiers aleatoires tries", std::is_sorted(grand.begin(), grand.end()));
    verifier("resultat identique à std::sort", grand == reference);

    std::cout << "\n" << (echecs == 0 ? "Tous les tests sont passes." :
                                         "Des tests ont echoue !") << "\n";

    // Décommenter pour voir le concept rejeter un type non triable :
    // std::vector<NonTriable> nt(3);
    // quickSort(nt);    // erreur : contraintes non satisfaites (Sortable)

    return echecs == 0 ? 0 : 1;
}
