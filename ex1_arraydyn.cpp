// Exercice 1 : conteneur dynamique générique avec itérateur personnalisé

#include <cstddef>
#include <iostream>
#include <iterator>     // uniquement pour les "tags" d'itérateur (bidirectional_iterator_tag)
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

template <typename T>
class ArrayDyn {
private:
    T* data_;
    std::size_t size_;
    std::size_t capacity_;

    // Réalloue le stockage interne (déplace les éléments existants)
    void reallocate(std::size_t newCapacity) {
        T* newData = new T[newCapacity];
        for (std::size_t i = 0; i < size_; ++i) {
            newData[i] = std::move(data_[i]);
        }
        delete[] data_;
        data_ = newData;
        capacity_ = newCapacity;
    }

public:
    // ------------------------------------------------------------------
    // Itérateur bidirectionnel.
    // IteratorT<T> sert pour begin()/end() et IteratorT<const T> pour les
    // versions const : on n'écrit le code qu'une seule fois.
    // ------------------------------------------------------------------
    template <typename U>
    class IteratorT {
    private:
        U* ptr_;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = std::remove_const_t<U>;
        using difference_type   = std::ptrdiff_t;
        using pointer           = U*;
        using reference         = U&;

        explicit IteratorT(U* p = nullptr) : ptr_(p) {}

        reference operator*() const { return *ptr_; }
        pointer operator->() const { return ptr_; }

        IteratorT& operator++() { ++ptr_; return *this; }              // ++it
        IteratorT operator++(int) { IteratorT t(*this); ++ptr_; return t; }  // it++
        IteratorT& operator--() { --ptr_; return *this; }              // --it
        IteratorT operator--(int) { IteratorT t(*this); --ptr_; return t; }  // it--

        bool operator==(const IteratorT& o) const { return ptr_ == o.ptr_; }
        bool operator!=(const IteratorT& o) const { return ptr_ != o.ptr_; }
    };

    using Iterator = IteratorT<T>;
    using ConstIterator = IteratorT<const T>;

    // ------------------------------------------------------------------
    // Constructeurs / destructeur / affectation (rule of three)
    // ------------------------------------------------------------------
    ArrayDyn() : data_(nullptr), size_(0), capacity_(0) {}

    explicit ArrayDyn(std::size_t capacity)
        : data_(capacity ? new T[capacity] : nullptr), size_(0), capacity_(capacity) {}

    ArrayDyn(const ArrayDyn& other)
        : data_(other.capacity_ ? new T[other.capacity_] : nullptr),
          size_(other.size_),
          capacity_(other.capacity_) {
        for (std::size_t i = 0; i < size_; ++i) {
            data_[i] = other.data_[i];
        }
    }

    ArrayDyn& operator=(const ArrayDyn& other) {
        if (this != &other) {                       // protection auto-affectation
            T* newData = other.capacity_ ? new T[other.capacity_] : nullptr;
            for (std::size_t i = 0; i < other.size_; ++i) {
                newData[i] = other.data_[i];
            }
            delete[] data_;                         // on libère l'ancien après la copie réussie
            data_ = newData;
            size_ = other.size_;
            capacity_ = other.capacity_;
        }
        return *this;
    }

    ~ArrayDyn() { delete[] data_; }

    // ------------------------------------------------------------------
    // Modificateurs
    // ------------------------------------------------------------------
    void push_back(const T& value) {
        if (size_ == capacity_) {
            // Copie préalable : "value" peut être une référence vers un élément
            // de ce tableau, qui deviendrait invalide après réallocation.
            T tmp(value);
            reallocate(capacity_ == 0 ? 1 : capacity_ * 2);
            data_[size_++] = std::move(tmp);
        } else {
            data_[size_++] = value;
        }
    }

    void pop_back() {
        if (size_ == 0) {
            throw std::out_of_range("pop_back() sur un conteneur vide");
        }
        --size_;
        data_[size_] = T{};   // libère les ressources éventuelles (ex. std::string)
    }

    // ------------------------------------------------------------------
    // Accès avec contrôle de bornes
    // ------------------------------------------------------------------
    T& operator[](std::size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index " + std::to_string(index) +
                                    " hors bornes (taille = " + std::to_string(size_) + ")");
        }
        return data_[index];
    }

    const T& operator[](std::size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Index " + std::to_string(index) +
                                    " hors bornes (taille = " + std::to_string(size_) + ")");
        }
        return data_[index];
    }

    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }

    // ------------------------------------------------------------------
    // begin() / end() : indispensables pour le range-based for
    // ------------------------------------------------------------------
    Iterator begin() { return Iterator(data_); }
    Iterator end() { return Iterator(data_ + size_); }
    ConstIterator begin() const { return ConstIterator(data_); }
    ConstIterator end() const { return ConstIterator(data_ + size_); }
};

// ======================================================================
// Programme de test
// ======================================================================

// Objet "maison"
struct Personne {
    std::string nom;
    int age;
    Personne() : nom(""), age(0) {}
    Personne(const std::string& n, int a) : nom(n), age(a) {}
};

std::ostream& operator<<(std::ostream& os, const Personne& p) {
    return os << p.nom << " (" << p.age << " ans)";
}

template <typename C>
void afficher(const std::string& titre, const C& conteneur) {
    std::cout << titre << " [taille=" << conteneur.size()
              << ", capacite=" << conteneur.capacity() << "] : ";
    for (const auto& e : conteneur) {      // utilise begin()/end() const
        std::cout << e << "  ";
    }
    std::cout << "\n";
}

int main() {
    // --- 1. int ---
    std::cout << "=== int ===\n";
    ArrayDyn<int> nombres;
    for (int i = 1; i <= 10; ++i) {
        nombres.push_back(i * i);
    }
    afficher("Carres", nombres);

    for (auto& e : nombres) {              // modification via auto&
        e *= 2;
    }
    afficher("Doubles", nombres);

    // Parcours à l'envers avec l'opérateur --
    std::cout << "A l'envers : ";
    auto it = nombres.end();
    while (it != nombres.begin()) {
        --it;
        std::cout << *it << "  ";
    }
    std::cout << "\n";

    // --- 2. std::string ---
    std::cout << "\n=== std::string ===\n";
    ArrayDyn<std::string> mots(2);         // capacité initiale 2 -> croissance automatique
    mots.push_back("Templates");
    mots.push_back("en");
    mots.push_back("C++");
    mots.push_back("moderne");
    afficher("Mots", mots);

    // --- 3. objet maison ---
    std::cout << "\n=== Personne ===\n";
    ArrayDyn<Personne> equipe;
    equipe.push_back(Personne("Alice", 25));
    equipe.push_back(Personne("Bob", 30));
    equipe.push_back(Personne("Charlie", 28));
    for (auto& p : equipe) {
        std::cout << "  " << p << "\n";
    }

    // --- 4. rule of three : copie profonde ---
    std::cout << "\n=== Copie et affectation ===\n";
    ArrayDyn<int> copie(nombres);          // constructeur de copie
    copie[0] = 999;
    afficher("Original", nombres);
    afficher("Copie   ", copie);

    ArrayDyn<int> affecte;
    affecte = nombres;                     // opérateur d'affectation
    affecte[1] = -1;
    afficher("Original", nombres);
    afficher("Affecté ", affecte);

    affecte = affecte;                     // auto-affectation sans danger
    afficher("Auto-aff", affecte);

    // --- 5. pop_back ---
    std::cout << "\n=== pop_back ===\n";
    nombres.pop_back();
    nombres.pop_back();
    afficher("Après 2 pop_back", nombres);

    // --- 6. contrôle des bornes ---
    std::cout << "\n=== Exceptions ===\n";
    try {
        std::cout << nombres[100] << "\n";
    } catch (const std::out_of_range& e) {
        std::cout << "Exception capturee : " << e.what() << "\n";
    }

    ArrayDyn<int> vide;
    try {
        vide.pop_back();
    } catch (const std::out_of_range& e) {
        std::cout << "Exception capturee : " << e.what() << "\n";
    }

    return 0;
}
