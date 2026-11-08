// Exercice 2 : matrice générique + opérateurs arithmétiques

#include <cstddef>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <type_traits>

// ----------------------------------------------------------------------
// Politique d'opérations élémentaires.
// Cas général : addition et multiplication usuelles.
// Spécialisation pour bool : "addition" = OU logique, "multiplication" = ET logique
// => le produit matriciel devient le produit booléen (OU des ET).
// ----------------------------------------------------------------------
template <typename T>
struct OpsMatrice {
    static T zero() { return T{}; }
    static T add(const T& a, const T& b) { return a + b; }
    static T mul(const T& a, const T& b) { return a * b; }
};

template <>
struct OpsMatrice<bool> {
    static bool zero() { return false; }
    static bool add(bool a, bool b) { return a || b; }
    static bool mul(bool a, bool b) { return a && b; }
};

// ----------------------------------------------------------------------
// Matrice<T, N, M> : N lignes, M colonnes, stockage statique
// ----------------------------------------------------------------------
template <typename T, std::size_t N, std::size_t M>
class Matrice {
private:
    T data_[N][M];

public:
    // Constructeur par défaut : tout à 0 (false pour bool)
    Matrice() : data_{} {}

    // Constructeur par liste d'initialisation, lecture ligne par ligne
    Matrice(std::initializer_list<T> valeurs) : data_{} {
        if (valeurs.size() > N * M) {
            throw std::invalid_argument("Trop de valeurs pour une matrice " +
                                        std::to_string(N) + "x" + std::to_string(M));
        }
        std::size_t k = 0;
        for (const T& v : valeurs) {
            data_[k / M][k % M] = v;
            ++k;
        }
    }

    // Accès (i, j) avec contrôle des bornes
    T& operator()(std::size_t i, std::size_t j) {
        if (i >= N || j >= M) throw std::out_of_range("Indice de matrice invalide");
        return data_[i][j];
    }
    const T& operator()(std::size_t i, std::size_t j) const {
        if (i >= N || j >= M) throw std::out_of_range("Indice de matrice invalide");
        return data_[i][j];
    }

    // Addition élément par élément
    Matrice operator+(const Matrice& autre) const {
        Matrice r;
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = 0; j < M; ++j)
                r.data_[i][j] = OpsMatrice<T>::add(data_[i][j], autre.data_[i][j]);
        return r;
    }

    // Soustraction élément par élément (sans sens pour bool -> erreur de compilation)
    Matrice operator-(const Matrice& autre) const {
        static_assert(!std::is_same_v<T, bool>, "operator- n'a pas de sens pour Matrice<bool>");
        Matrice r;
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = 0; j < M; ++j)
                r.data_[i][j] = data_[i][j] - autre.data_[i][j];
        return r;
    }

    // Produit : (N x M) * (M x P) -> (N x P)
    // La compatibilité des dimensions est vérifiée À LA COMPILATION : multiplier
    // une 2x3 par une 2x2 ne compile tout simplement pas.
    template <std::size_t P>
    Matrice<T, N, P> operator*(const Matrice<T, M, P>& autre) const {
        Matrice<T, N, P> r;
        for (std::size_t i = 0; i < N; ++i) {
            for (std::size_t j = 0; j < P; ++j) {
                T acc = OpsMatrice<T>::zero();
                for (std::size_t k = 0; k < M; ++k) {
                    acc = OpsMatrice<T>::add(acc, OpsMatrice<T>::mul(data_[i][k], autre(k, j)));
                }
                r(i, j) = acc;
            }
        }
        return r;
    }

    bool operator==(const Matrice& autre) const {
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = 0; j < M; ++j)
                if (!(data_[i][j] == autre.data_[i][j])) return false;
        return true;
    }

    // Fonction amie définie dans la classe : affichage formaté
    friend std::ostream& operator<<(std::ostream& os, const Matrice& m) {
        for (std::size_t i = 0; i < N; ++i) {
            os << "| ";
            for (std::size_t j = 0; j < M; ++j) {
                os << std::setw(7) << m.data_[i][j] << ' ';
            }
            os << "|\n";
        }
        return os;
    }
};

// ======================================================================
// Programme de test
// ======================================================================
int main() {
    // --- Matrices d'entiers ---
    std::cout << "=== int ===\n";
    Matrice<int, 2, 3> A = {1, 2, 3,
                            4, 5, 6};
    Matrice<int, 2, 3> B = {6, 5, 4,
                            3, 2, 1};
    Matrice<int, 3, 2> C = {1, 2,
                            3, 4,
                            5, 6};

    std::cout << "A =\n" << A;
    std::cout << "B =\n" << B;
    std::cout << "C =\n" << C;

    std::cout << "A + B =\n" << (A + B);
    std::cout << "A - B =\n" << (A - B);
    std::cout << "A * C (2x3 * 3x2 = 2x2) =\n" << (A * C);
    std::cout << "C * A (3x2 * 2x3 = 3x3) =\n" << (C * A);

    // A * B ne compile pas (2x3 * 2x3) : décommenter pour voir l'erreur
    // std::cout << (A * B);

    // Vérification avec la matrice identité : A * I = A
    Matrice<int, 3, 3> I = {1, 0, 0,
                            0, 1, 0,
                            0, 0, 1};
    std::cout << "A * I == A ? " << ((A * I) == A ? "oui" : "NON") << "\n";

    Matrice<int, 2, 2> zero;               // constructeur par défaut
    std::cout << "Matrice nulle 2x2 =\n" << zero;

    // --- Matrices de double ---
    std::cout << "\n=== double ===\n";
    std::cout << std::fixed << std::setprecision(2);
    Matrice<double, 2, 2> D = {1.5, 2.0,
                               0.5, 4.0};
    Matrice<double, 2, 2> E = {2.0, 0.0,
                               1.0, 0.5};
    std::cout << "D =\n" << D;
    std::cout << "E =\n" << E;
    std::cout << "D + E =\n" << (D + E);
    std::cout << "D - E =\n" << (D - E);
    std::cout << "D * E =\n" << (D * E);
    std::cout << std::defaultfloat;

    // --- Spécialisation bool : produit logique ---
    std::cout << "\n=== bool (produit logique : OU des ET) ===\n";
    std::cout << std::boolalpha;
    Matrice<bool, 2, 2> P = {true,  false,
                             true,  true};
    Matrice<bool, 2, 2> Q = {false, true,
                             true,  false};
    std::cout << "P =\n" << P;
    std::cout << "Q =\n" << Q;
    std::cout << "P * Q =\n" << (P * Q);
    std::cout << "P + Q (OU) =\n" << (P + Q);
    // P - Q : erreur de compilation volontaire grâce au static_assert

    // --- Gestion d'erreur ---
    std::cout << "\n=== Exceptions ===\n";
    try {
        std::cout << A(5, 5) << "\n";
    } catch (const std::out_of_range& e) {
        std::cout << "Exception capturee : " << e.what() << "\n";
    }
    try {
        Matrice<int, 1, 2> trop = {1, 2, 3};
        (void)trop;
    } catch (const std::invalid_argument& e) {
        std::cout << "Exception capturee : " << e.what() << "\n";
    }

    return 0;
}
