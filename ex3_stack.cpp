// Exercice 3 : pile générique sécurisée avec std::unique_ptr
#include <cstddef>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ----------------------------------------------------------------------
// Exceptions spécifiques
// ----------------------------------------------------------------------
class StackOverflow : public std::runtime_error {
public:
    StackOverflow() : std::runtime_error("StackOverflow : la pile est pleine") {}
};

class StackUnderflow : public std::runtime_error {
public:
    StackUnderflow() : std::runtime_error("StackUnderflow : la pile est vide") {}
};

// ----------------------------------------------------------------------
// Stack<T> : capacité fixe, mémoire gérée par unique_ptr<T[]> (pas de delete[] à écrire)
// ----------------------------------------------------------------------
template <typename T>
class Stack {
private:
    std::size_t capacity_;
    std::size_t top_;                 // nombre d'éléments = indice du prochain emplacement libre
    std::unique_ptr<T[]> data_;

    static std::unique_ptr<T[]> allocate(std::size_t n) {
        return n ? std::make_unique<T[]>(n) : nullptr;
    }

public:
    // Constructeur avec capacité par défaut
    explicit Stack(std::size_t capacity = 16)
        : capacity_(capacity), top_(0), data_(allocate(capacity)) {}

    // Constructeur à partir d'une collection existante (begin, end)
    // La capacité est égale au nombre d'éléments de la collection.
    template <typename InputIt,
              typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
    Stack(InputIt first, InputIt last)
        : capacity_(static_cast<std::size_t>(std::distance(first, last))),
          top_(0),
          data_(allocate(capacity_)) {
        for (; first != last; ++first) {
            data_[top_++] = *first;
        }
    }

    // unique_ptr n'est pas copiable : on interdit explicitement la copie...
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    // ...mais on autorise le déplacement (la pile source est laissée vide et valide)
    Stack(Stack&& other) noexcept
        : capacity_(std::exchange(other.capacity_, 0)),
          top_(std::exchange(other.top_, 0)),
          data_(std::move(other.data_)) {}

    Stack& operator=(Stack&& other) noexcept {
        if (this != &other) {
            capacity_ = std::exchange(other.capacity_, 0);
            top_ = std::exchange(other.top_, 0);
            data_ = std::move(other.data_);
        }
        return *this;
    }

    // Le destructeur par défaut suffit : unique_ptr libère le tableau automatiquement.

    void push(const T& value) {
        if (top_ == capacity_) throw StackOverflow();
        data_[top_++] = value;
    }

    void pop() {
        if (top_ == 0) throw StackUnderflow();
        --top_;
        data_[top_] = T{};            // libère les ressources détenues par l'élément retiré
    }

    const T& peek() const {
        if (top_ == 0) throw StackUnderflow();
        return data_[top_ - 1];
    }

    bool empty() const { return top_ == 0; }
    std::size_t size() const { return top_; }
    std::size_t capacity() const { return capacity_; }
};

// ======================================================================
// Programme de test
// ======================================================================

// Objet "traceur" : compte les instances vivantes pour détecter toute fuite
struct Tracker {
    static inline int live = 0;
    Tracker() { ++live; }
    Tracker(const Tracker&) { ++live; }
    Tracker& operator=(const Tracker&) = default;
    ~Tracker() { --live; }
};

int main() {
    // --- 1. Fonctionnement de base ---
    std::cout << "=== Fonctionnement de base ===\n";
    Stack<int> s(3);
    s.push(10);
    s.push(20);
    s.push(30);
    std::cout << "Sommet : " << s.peek() << " | taille : " << s.size() << "\n";
    s.pop();
    std::cout << "Apres pop, sommet : " << s.peek() << "\n";
    std::cout << "Vide ? " << std::boolalpha << s.empty() << "\n";

    // --- 2. Exceptions ---
    std::cout << "\n=== Exceptions ===\n";
    try {
        Stack<int> petite(2);
        petite.push(1);
        petite.push(2);
        petite.push(3);                // dépasse la capacité
    } catch (const StackOverflow& e) {
        std::cout << "Capture : " << e.what() << "\n";
    }

    try {
        Stack<std::string> vide;
        vide.pop();
    } catch (const StackUnderflow& e) {
        std::cout << "Capture : " << e.what() << "\n";
    }

    try {
        Stack<int> vide;
        std::cout << vide.peek() << "\n";
    } catch (const StackUnderflow& e) {
        std::cout << "Capture : " << e.what() << "\n";
    }

    // --- 3. Construction depuis une collection (begin, end) ---
    std::cout << "\n=== Construction depuis une collection ===\n";
    std::vector<std::string> mots = {"un", "deux", "trois", "quatre"};
    Stack<std::string> pileMots(mots.begin(), mots.end());
    std::cout << "Depuis vector -> taille " << pileMots.size()
              << ", sommet : " << pileMots.peek() << "\n";

    std::list<int> liste = {5, 6, 7};
    Stack<int> pileListe(liste.begin(), liste.end());
    std::cout << "Depuis list   -> taille " << pileListe.size()
              << ", sommet : " << pileListe.peek() << "\n";

    std::cout << "Depilage de pileMots : ";
    while (!pileMots.empty()) {
        std::cout << pileMots.peek() << " ";
        pileMots.pop();
    }
    std::cout << "\n";

    // --- 4. Sémantique de déplacement ---
    std::cout << "\n=== Deplacement ===\n";
    Stack<int> a(5);
    a.push(42);
    Stack<int> b = std::move(a);
    std::cout << "b.peek() = " << b.peek() << " | a.empty() = " << a.empty() << "\n";

    // --- 5. Gros volume ---
    std::cout << "\n=== Gros volume ===\n";
    {
        const std::size_t N = 1'000'000;
        Stack<int> grosse(N);
        for (std::size_t i = 0; i < N; ++i) grosse.push(static_cast<int>(i));
        long long somme = 0;
        while (!grosse.empty()) {
            somme += grosse.peek();
            grosse.pop();
        }
        std::cout << "1 000 000 int empiles/depiles, somme = " << somme << "\n";

        Stack<std::string> chaines(200'000);
        for (std::size_t i = 0; i < 200'000; ++i) chaines.push("element_" + std::to_string(i));
        std::cout << "200 000 string empilees, sommet : " << chaines.peek() << "\n";
    }   // destruction automatique ici, sans delete[]

    // --- 6. Détection de fuites (compteur d'instances) ---
    std::cout << "\n=== Verification des fuites ===\n";
    {
        Stack<Tracker> t(1000);
        std::cout << "Instances vivantes apres construction : " << Tracker::live << "\n";
        try {
            for (int i = 0; i < 2000; ++i) t.push(Tracker{});   // finit par déborder
        } catch (const StackOverflow&) {
            std::cout << "Debordement attrape, pile pleine (" << t.size() << " elements)\n";
        }
    }
    std::cout << "Instances vivantes apres destruction : " << Tracker::live
              << (Tracker::live == 0 ? "  -> aucune fuite" : "  -> FUITE !") << "\n";

    return 0;
}
