// MenuModel.hpp -- YOUR submission for PA 1,2.  (Header-only.)
//
// PA 1,2 builds on PA 0. The PA 0 types (Course, ChosenCourse, PairingFactor,
// MenuModel) are PROVIDED below, fully working -- you do NOT need to
// re-implement them. Your job is the PA 1,2 section at the bottom, marked with
// // TODO:
//   * class Meal                  -- finish its rule of five (the allocating
//   constructor,
//                                    destructor, and copy constructor).
//   * MenuModel::consistentMealCount()      -- count the meals consistent with
//   the chosen courses (a method).
//   * MenuModel::MealIterator::operator*   -- the decode that makes
//   model.completeMeals() yield each meal.
//   * fillCompleteMeals           -- ITERATE model.completeMeals() to FILL a
//   caller-provided array.
//   * Meal::compareMeals (static) -- the ordering used by the sort and the
//   binary search.
//   * Meal::operator==           -- do two meals match? (order-independent;
//   used by the linear search).
//   * binarySearchForMeal         -- a binary search over a sorted Meal array.
//   * mergeSortMeals                 -- a recursive MERGE SORT (Unit 2); the
//   fast counterpart to
//                                       the provided O(N^2) bubbleSortMeals.
// bubbleSortMeals, linearSearchForMeal (which uses your Meal::operator==), and
// operator* are PROVIDED -- you USE / read them, then analyze their cost (you
// do NOT write them).
//
// RULES
//   * Header-only: put everything in this file. Do NOT modify driver.cpp.
//   * Standard library is allowed ONLY for input: <fstream>, <string>,
//   <stdexcept>, <utility>.
//     Do NOT use std::vector / std::map / other containers -- use raw new[] /
//     delete[].
//   * Private data members use a LEADING underscore (e.g. _choices).
//   * Any class that owns raw memory needs a correct rule of five.
//   * Recursion is fine to use wherever it helps -- mergeSortMeals is naturally
//   recursive.
//   * Be ready to ANALYZE the time and space complexity (Big-O) of each
//   function you write.
#ifndef MENU_MODEL_HPP
#define MENU_MODEL_HPP

#include <algorithm>
#include <cassert>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility> // std::swap

// One course in the menu: its index (its position 0, 1, 2, ...) and how many
// dishes it offers.
struct Course {
    int courseIdx;
    int numDishes;
};

// One course already chosen in advance (read from the .chosen file): which
// course, which dish.
struct ChosenCourse {
    int courseIdx;
    int dish;
};

// ===========================================================================
//  PROVIDED -- the PA 0 model, complete. (Read it; you build on top of it.)
// ===========================================================================
class PairingFactor {
  private:
    int _scopeSize;
    Course const **_scope; // const pointers to the courses this table covers
                           // (owned by MenuModel)
    long long _numEntries;
    double *_scores;

  public:
    PairingFactor()
        : _scopeSize(0), _scope(nullptr), _numEntries(0), _scores(nullptr) {}

    void allocateScope(int scopeSize) {
        _scopeSize = scopeSize;
        _scope = new Course const *[scopeSize];
    }
    void setCourse(int index, Course const *course) { _scope[index] = course; }

    void allocateScores(long long numEntries) {
        _numEntries = numEntries;
        _scores = new double[numEntries];
    }
    void setScore(long long index, double score) { _scores[index] = score; }

    ~PairingFactor() {
        delete[] _scope;
        delete[] _scores;
    }

    PairingFactor(PairingFactor const &other)
        : _scopeSize(other._scopeSize), _scope(nullptr),
          _numEntries(other._numEntries), _scores(nullptr) {
        if (_scopeSize > 0) {
            _scope = new Course const *[_scopeSize];
            for (int i = 0; i < _scopeSize; ++i)
                _scope[i] = other._scope[i];
        }
        if (_numEntries > 0) {
            _scores = new double[_numEntries];
            for (long long i = 0; i < _numEntries; ++i)
                _scores[i] = other._scores[i];
        }
    }

    friend void swap(PairingFactor &a, PairingFactor &b) noexcept {
        std::swap(a._scopeSize, b._scopeSize);
        std::swap(a._scope, b._scope);
        std::swap(a._numEntries, b._numEntries);
        std::swap(a._scores, b._scores);
    }
    PairingFactor(PairingFactor &&other) noexcept : PairingFactor() {
        swap(*this, other);
    }
    PairingFactor &operator=(PairingFactor other) {
        swap(*this, other);
        return *this;
    }

    int scopeSize() const { return _scopeSize; }
    int courseAt(int index) const { return _scope[index]->courseIdx; }
    long long numEntries() const { return _numEntries; }
    double scoreAt(long long index) const { return _scores[index]; }

    // PA 1: operator* (defined at the bottom) is a friend so it can read these
    // scope pointers.
    friend PairingFactor operator*(PairingFactor const &a,
                                   PairingFactor const &b);
};

class Meal; // forward declaration (the class is defined below) so MealIterator
            // can name it

class MenuModel {
  private:
    int _numCourses;
    Course *_courses;
    int _numFactors;
    PairingFactor *_factors;
    int _numChosen;
    ChosenCourse *_chosen;

  public:
    MenuModel()
        : _numCourses(0), _courses(nullptr), _numFactors(0), _factors(nullptr),
          _numChosen(0), _chosen(nullptr) {}

    explicit MenuModel(std::string const &menuPath) : MenuModel() {
        _loadMenu(menuPath);
    }

    ~MenuModel() {
        delete[] _courses;
        delete[] _factors;
        delete[] _chosen;
    }

    MenuModel(MenuModel const &other)
        : _numCourses(other._numCourses), _courses(nullptr),
          _numFactors(other._numFactors), _factors(nullptr),
          _numChosen(other._numChosen), _chosen(nullptr) {
        if (_numCourses > 0) {
            _courses = new Course[_numCourses];
            for (int i = 0; i < _numCourses; ++i)
                _courses[i] = other._courses[i];
        }
        if (_numFactors > 0) {
            _factors = new PairingFactor[_numFactors];
            for (int i = 0; i < _numFactors; ++i) {
                _factors[i] = other._factors[i];
                for (int j = 0; j < _factors[i].scopeSize(); ++j)
                    _factors[i].setCourse(j,
                                          &_courses[_factors[i].courseAt(j)]);
            }
        }
        if (_numChosen > 0) {
            _chosen = new ChosenCourse[_numChosen];
            for (int i = 0; i < _numChosen; ++i)
                _chosen[i] = other._chosen[i];
        }
    }

    friend void swap(MenuModel &a, MenuModel &b) noexcept {
        std::swap(a._numCourses, b._numCourses);
        std::swap(a._courses, b._courses);
        std::swap(a._numFactors, b._numFactors);
        std::swap(a._factors, b._factors);
        std::swap(a._numChosen, b._numChosen);
        std::swap(a._chosen, b._chosen);
    }
    MenuModel(MenuModel &&other) noexcept : MenuModel() { swap(*this, other); }
    MenuModel &operator=(MenuModel other) {
        swap(*this, other);
        return *this;
    }

    void readChosen(std::string const &chosenPath) {
        std::ifstream in(chosenPath);
        if (!in)
            throw std::runtime_error("cannot open " + chosenPath);
        int k;
        if (!(in >> k))
            throw std::runtime_error("bad .chosen header");
        delete[] _chosen;
        _numChosen = k;
        _chosen = new ChosenCourse[k];
        for (int i = 0; i < k; ++i)
            in >> _chosen[i].courseIdx >> _chosen[i].dish;
    }

    int numCourses() const { return _numCourses; }
    int dishes(int courseIdx) const { return _courses[courseIdx].numDishes; }
    int numTables() const { return _numFactors; }
    long long
    totalMealCount() const { // every complete meal, IGNORING the chosen courses
        long long product = 1;
        for (int i = 0; i < _numCourses; ++i)
            product *= _courses[i].numDishes;
        return product;
    }
    int chosenCount() const { return _numChosen; }
    int freeCourses() const { return _numCourses - _numChosen; }
    int chosenCourseAt(int i) const { return _chosen[i].courseIdx; }
    int chosenDishAt(int i) const { return _chosen[i].dish; }

    // ---- PA 1,2 chosen-course helpers (PROVIDED, now methods) ---------------
    bool isChosen(int courseIdx) const {
        for (int i = 0; i < _numChosen; ++i)
            if (_chosen[i].courseIdx == courseIdx)
                return true;
        return false;
    }
    int chosenDish(int courseIdx) const {
        for (int i = 0; i < _numChosen; ++i)
            if (_chosen[i].courseIdx == courseIdx)
                return _chosen[i].dish;
        return -1; // course is not chosen
    }
    // How many complete meals are consistent with the chosen courses: the
    // product of the FREE courses' dish counts (each chosen course contributes
    // x1). Used by completeMeals()/fillCompleteMeals below.
    long long consistentMealCount() const {
        // DONE: return that count (defined above).
        long long prod{1};
        for (int i{0}; i < _numCourses; ++i) {
            prod *= _courses[i].numDishes;
        }

        for (int i{0}; i < _numChosen; ++i) {
            prod /= _courses[_chosen[i].courseIdx].numDishes;
        }
        return prod;
    }

    class Iterator {
      public:
        explicit Iterator(PairingFactor const *ptr) : _ptr(ptr) {}
        PairingFactor const &operator*() const { return *_ptr; }
        Iterator &operator++() {
            ++_ptr;
            return *this;
        }
        bool operator!=(Iterator const &other) const {
            return _ptr != other._ptr;
        }

      private:
        PairingFactor const *_ptr;
    };
    Iterator begin() const { return Iterator(_factors); }
    Iterator end() const { return Iterator(_factors + _numFactors); }
    PairingFactor const &factorAt(int i) const { return _factors[i]; }

    // ---- PA 1,2: a LAZY iterator over the complete meals consistent with the
    // chosen courses ------ model.completeMeals() yields each meal one at a
    // time (no giant array) -- ITERATE it to visit or COLLECT the meals. The
    // meal at position `pos` fixes the chosen courses and decodes `pos` across
    // the FREE courses like an odometer. PROVIDED scaffolding -- you finish
    // operator* (defined below).
    class MealIterator {
      public:
        MealIterator(MenuModel const *model, long long pos)
            : _model(model), _pos(pos) {}
        Meal operator*() const; // YOU write this (defined after Meal)
        MealIterator &operator++() {
            ++_pos;
            return *this;
        }
        bool operator!=(MealIterator const &other) const {
            return _pos != other._pos;
        }

      private:
        MenuModel const *_model;
        long long _pos;
    };
    class CompleteMeals { // a tiny range: begin() .. end()
      public:
        explicit CompleteMeals(MenuModel const *model) : _model(model) {}
        MealIterator begin() const { return MealIterator(_model, 0); }
        MealIterator end() const {
            return MealIterator(_model, _model->consistentMealCount());
        }

      private:
        MenuModel const *_model;
    };
    CompleteMeals completeMeals() const { return CompleteMeals(this); }

  private:
    void _loadMenu(std::string const &menuPath) {
        std::ifstream in(menuPath);
        if (!in)
            throw std::runtime_error("cannot open " + menuPath);

        std::string tag;
        in >> tag; // "MENU" -- a format marker; skip it

        in >> _numCourses;
        _courses = new Course[_numCourses];
        for (int i = 0; i < _numCourses; ++i) {
            _courses[i].courseIdx = i;
            in >> _courses[i].numDishes;
        }

        in >> _numFactors;
        _factors = new PairingFactor[_numFactors];
        // Each scope lists its course ids in ASCENDING order (a .menu format
        // rule), so every factor's _scope ends up sorted -- you may rely on
        // that (operator*'s union merge does).
        for (int i = 0; i < _numFactors; ++i) {
            int scopeSize;
            in >> scopeSize;
            _factors[i].allocateScope(scopeSize);
            for (int j = 0; j < scopeSize; ++j) {
                int courseIdx;
                in >> courseIdx;
                _factors[i].setCourse(j, &_courses[courseIdx]);
            }
        }
        for (int i = 0; i < _numFactors; ++i) {
            long long numEntries;
            in >> numEntries;
            _factors[i].allocateScores(numEntries);
            for (long long j = 0; j < numEntries; ++j) {
                double score;
                in >> score;
                _factors[i].setScore(j, score);
            }
        }
    }
};

// ===========================================================================
//  PA 1 -- YOUR WORK STARTS HERE
// ===========================================================================

// ---------------------------------------------------------------------------
// A complete MEAL: one chosen dish for every course (a full assignment). It
// OWNS a raw array of ChosenCourse (one per course), so it needs the rule of
// five -- just like the classes above. (swap, the move constructor, the
// assignment operator, and the accessors are PROVIDED; finish
//  the three TODO members.)
// ---------------------------------------------------------------------------
class Meal {
  private:
    int _numCourses;
    ChosenCourse *_choices; // _choices[c] = { courseIdx = c, dish = the dish
                            // picked for course c }

  public:
    Meal() : _numCourses(0), _choices(nullptr) {}

    explicit Meal(int numCourses) : _numCourses(numCourses), _choices(nullptr) {
        // DONE: make this meal hold one dish-slot per course (0 ..
        // numCourses-1), with no dish chosen
        //       yet -- use -1 as the "not set" placeholder. (setDish fills the
        //       real dishes in later.)
        _choices = new ChosenCourse[numCourses];
        for (int i{0}; i < _numCourses; ++i) {
            _choices[i].courseIdx = i;
            _choices[i].dish = -1;
        }
    }

    ~Meal() {
        // DONE: release the memory this meal owns.
        delete[] _choices;
    }

    Meal(Meal const &other)
        : _numCourses(other._numCourses),
          _choices(new ChosenCourse[other._numCourses]) {
        // DONE: make this an independent DEEP copy of other (its own memory,
        // the same dishes) so the
        //       two meals never share storage.
        for (int i{0}; i < _numCourses; ++i) {
            _choices[i].courseIdx = other._choices[i].courseIdx;
            _choices[i].dish = other._choices[i].dish;
        }
    }

    // PROVIDED: swap + the copy-and-swap move/assignment.
    friend void swap(Meal &a, Meal &b) noexcept {
        std::swap(a._numCourses, b._numCourses);
        std::swap(a._choices, b._choices);
    }
    Meal(Meal &&other) noexcept : Meal() { swap(*this, other); }
    Meal &operator=(Meal other) {
        swap(*this, other);
        return *this;
    }

    // PROVIDED accessors.
    void setDish(int courseIdx, int dish) { _choices[courseIdx].dish = dish; }
    int dishFor(int courseIdx) const { return _choices[courseIdx].dish; }
    int size() const { return _numCourses; }
    ChosenCourse choiceAt(int i) const { return _choices[i]; }

    // STATIC comparator (it compares TWO meals, so neither is `this`): order a
    // and b lexicographically by the dish each picks, in the order given by
    // courseSortOrder. Return NEGATIVE if a < b, ZERO if they match on every
    // course, POSITIVE if a > b. The shared helper for BOTH the sort and the
    // search; call sites use it as Meal::compareMeals(a, b, order, n).
    static int compareMeals(Meal const &a, Meal const &b,
                            Course const *courseSortOrder, int nCourses) {
        // DONE: return a negative / zero / positive result per the ordering
        // described above.

        for (int i{0}; i < nCourses; ++i) {
            int courseIdx = courseSortOrder[i].courseIdx;
            int difference = a.dishFor(courseIdx) - b.dishFor(courseIdx);
            if (difference != 0)
                return difference;
        }
        return 0;
    }

    // operator== : do two meals pick the same dish on EVERY course? Equality
    // needs NO courseSortOrder (unlike compareMeals), so it fits an operator's
    // fixed 2-operand signature -- an equality-only search (the linear scan)
    // uses  a == b  instead of the heavier comparator. (In C++20, defining ==
    // gives != free.)
    bool operator==(Meal const &other) const {
        // DONE: return whether the two meals pick the same dish on every
        // course.
        if (other._numCourses != _numCourses)
            return false;
        for (int i{0}; i < _numCourses; ++i) {
            if (other._choices[i].dish != _choices[i].dish) {
                return false;
            }
        }
        return true;
    }
};

// The meal at position `pos`: the chosen courses are fixed, and the FREE
// courses are a mixed-radix decode of `pos` (first free course slowest -- peel
// the LAST course's digit first). This decode is what makes
// model.completeMeals() yield each meal, so YOU write it.
inline Meal MenuModel::MealIterator::operator*() const {
    // DONE: build and return the complete meal at position _pos -- each chosen
    // course takes its fixed
    //       dish, and the free courses together encode _pos (a mixed-radix /
    //       odometer decode over the free courses; see the lecture's odometer).

    // Empty Meal with dish placeholders
    int numCourses{_model->numCourses()};
    Meal result = Meal(numCourses);

    // Fixing chosen dishes
    for (int i{0}; i < _model->chosenCount(); ++i) {
        int chosenCIdx = _model->chosenCourseAt(i);
        int chosenDish = _model->chosenDishAt(i);
        result.setDish(chosenCIdx, chosenDish);
    }

    long long remaining{_pos};

    for (int i{numCourses - 1}; i >= 0; --i) {
        // Excluding already fixed chosen dish
        if (result.dishFor(i) != -1)
            continue;
        int numDishes = _model->dishes(i);
        result.setDish(i, remaining % numDishes);
        remaining /= numDishes;
    }

    return result;
}

// Fill the caller's array `meals` (which must already have length
// model.consistentMealCount()) with EVERY complete meal consistent with the
// chosen courses, by ITERATING model.completeMeals(). The CALLER owns `meals`
// (it allocated it and frees it); this function allocates nothing. Be ready to
// analyze this function's time and space complexity (Big-O).
inline void fillCompleteMeals(MenuModel const &model, Meal *meals) {
    // DONE: fill meals[0 .. model.consistentMealCount()-1] with every complete
    // meal, by iterating
    //       model.completeMeals(). (The caller already sized the array;
    //       allocate nothing here.)

    int consistentMealCount = model.consistentMealCount();
    MenuModel::CompleteMeals container = model.completeMeals();
    int count{0};
    for (Meal meal : container) {
        meals[count++] = meal;
    }

    assert(consistentMealCount == count &&
           "Iterator produced an incorrect number of meals.");
}

// PROVIDED -- sort meals in place by bubble sort, ordering by
// Meal::compareMeals(...). O(nMealsToSort^2) comparisons; each comparison is
// O(nCourses). You implement a real (merge) sort below; here, USE this one as
// the O(N^2) contrast -- and be ready to analyze its complexity.
inline void bubbleSortMeals(Meal *mealsToSort, int nMealsToSort,
                            Course const *courseSortOrder, int nCourses) {
    for (int i = 0; i < nMealsToSort - 1; ++i)
        for (int j = 0; j < nMealsToSort - 1 - i; ++j)
            if (Meal::compareMeals(mealsToSort[j], mealsToSort[j + 1],
                                   courseSortOrder, nCourses) > 0)
                swap(mealsToSort[j], mealsToSort[j + 1]);
}

// =====================  YOUR Unit 2 work: a recursive MERGE SORT
// ===================== Recursively MERGE-SORT meals[0..n-1] IN PLACE, ordering
// by Meal::compareMeals(...) with the SAME courseSortOrder bubbleSortMeals uses
// (so it produces the IDENTICAL sorted array). Sort the two halves, then MERGE
// them using a single heap scratch buffer (raw new[] / delete[] -- NO
// std::vector / std::merge). Recurrence: T(N) = 2 T(N/2) + O(N*C)  =>  O(N * C
// * log N). Be ready to analyze its time AND space complexity (Big-O).
inline void mergeMeals(Meal *meals, int lo, int mid, int hi,
                       Course const *courseSortOrder, int nCourses,
                       Meal *scratch) {
    // TODO: merge the two already-sorted runs meals[lo..mid] and
    // meals[mid+1..hi] into one sorted run,
    //       via the scratch buffer. Keep it STABLE (equal meals stay in their
    //       original order) so the result matches bubbleSortMeals.
    (void)meals;
    (void)lo;
    (void)mid;
    (void)hi;
    (void)courseSortOrder;
    (void)nCourses;
    (void)scratch;
}
inline void mergeSortMeals(Meal *meals, int lo, int hi,
                           Course const *courseSortOrder, int nCourses,
                           Meal *scratch) {
    // TODO: recursively sort meals[lo..hi] -- sort each half, then combine them
    // with mergeMeals.
    (void)meals;
    (void)lo;
    (void)hi;
    (void)courseSortOrder;
    (void)nCourses;
    (void)scratch;
}
// Public entry point -- the signature the driver / autograder call (same shape
// as bubbleSortMeals).
inline void mergeSortMeals(Meal *meals, int n, Course const *courseSortOrder,
                           int nCourses) {
    // TODO: sort meals[0..n-1] via the recursive helper above, using ONE
    // scratch buffer that you
    //       allocate once and free -- every new[] needs a matching delete[]
    //       (the memory gate checks this).
    (void)meals;
    (void)n;
    (void)courseSortOrder;
    (void)nCourses;
}

// PROVIDED -- a simple LINEAR search: scan meals in order for the first one
// that is equal (==) to the target. Works on an UNSORTED array and needs NO
// sort order (equality is order-independent), so it does not take a
// courseSortOrder; O(nMeals * nCourses). Here to contrast its cost with the
// binary search below -- the Unit 1 lesson.
inline int linearSearchForMeal(Meal const *meals, int nMeals,
                               Meal const &target) {
    for (int i = 0; i < nMeals; ++i)
        if (meals[i] == target)
            return i;
    return -1;
}

// Binary search for `target` in sortedMeals (sorted by bubbleSortMeals with the
// SAME courseSortOrder). Return the index of a matching meal, or -1 if none
// matches. Be ready to analyze this function's time and space complexity
// (Big-O).
inline int binarySearchForMeal(Meal const *sortedMeals, int nSortedMeals,
                               Meal const &target,
                               Course const *courseSortOrder, int nCourses) {
    // TODO: return the index of a meal equal to target (compare with
    // Meal::compareMeals and this
    //       courseSortOrder), or -1 if none. The array is sorted -- search it
    //       in O(log N).
    (void)sortedMeals;
    (void)nSortedMeals;
    (void)target;
    (void)courseSortOrder;
    (void)nCourses;
    return -1;
}

// PROVIDED -- the PRODUCT of two pairing tables: a new factor over the UNION of
// the two scopes, where each entry = a's entry for the matching sub-assignment
// TIMES b's entry. Its size = the product of the union courses' dish counts (R,
// up to D^u). A non-member FRIEND of PairingFactor (reads a._scope/b._scope).
// You do NOT write this --- read it, be able to describe what it does, and
// analyze its complexity (Big-O).
//
// Cost: it must fill R entries, so it is Theta(R) = O(D^u) at best; this
// version is O(R * u) (u = union size: each entry decodes the assignment and
// indexes both factors). A maximally efficient O(R) variant would increment the
// union assignment like an odometer and update each factor's running index by
// +/- a precomputed place value only on the digit that changes (amortized
// O(1)/entry), instead of re-decoding e each step. We keep the simpler O(R * u)
// form here; R = D^u dominates either way.
inline PairingFactor operator*(PairingFactor const &a, PairingFactor const &b) {
    // Both input scopes are ASCENDING (a guaranteed .menu format rule), so the
    // two-pointer loop below merges them into the union in ascending order with
    // no duplicates.
    Course const **unionScope = new Course const *[a._scopeSize + b._scopeSize];
    int unionSize = 0;
    int ia = 0;
    int ib = 0;
    while (ia < a._scopeSize || ib < b._scopeSize) {
        Course const *next;
        if (ib >= b._scopeSize) {
            next = a._scope[ia++];
        } else if (ia >= a._scopeSize) {
            next = b._scope[ib++];
        } else if (a._scope[ia]->courseIdx < b._scope[ib]->courseIdx) {
            next = a._scope[ia++];
        } else if (b._scope[ib]->courseIdx < a._scope[ia]->courseIdx) {
            next = b._scope[ib++];
        } else {
            next = a._scope[ia++];
            ++ib;
        }
        if (unionSize == 0 ||
            unionScope[unionSize - 1]->courseIdx != next->courseIdx)
            unionScope[unionSize++] = next;
    }

    PairingFactor result;
    result.allocateScope(unionSize);
    long long numResultEntries = 1;
    for (int k = 0; k < unionSize; ++k) {
        result.setCourse(k, unionScope[k]);
        numResultEntries *= unionScope[k]->numDishes;
    }
    result.allocateScores(numResultEntries);

    // dishByCourse[courseIdx] = the dish chosen for that course in the current
    // union assignment, indexed by GLOBAL course id, so each factor can be
    // indexed straight from its own scope.
    int maxCourse = 0;
    for (int k = 0; k < unionSize; ++k)
        if (unionScope[k]->courseIdx > maxCourse)
            maxCourse = unionScope[k]->courseIdx;
    int *dishByCourse = new int[maxCourse + 1]();

    for (long long entryIndex = 0; entryIndex < numResultEntries;
         ++entryIndex) {
        // decode entryIndex into a dish for each union course (first union
        // course most significant)
        long long remaining = entryIndex;
        for (int k = unionSize - 1; k >= 0; --k) {
            int c = unionScope[k]->courseIdx;
            int nd = unionScope[k]->numDishes;
            dishByCourse[c] = static_cast<int>(remaining % nd);
            remaining /= nd;
        }
        // each factor's own row-major index, read directly from its own
        // (ascending) scope
        long long aIndex = 0;
        for (int j = 0; j < a._scopeSize; ++j) {
            int c = a._scope[j]->courseIdx;
            aIndex = aIndex * a._scope[j]->numDishes + dishByCourse[c];
        }
        long long bIndex = 0;
        for (int j = 0; j < b._scopeSize; ++j) {
            int c = b._scope[j]->courseIdx;
            bIndex = bIndex * b._scope[j]->numDishes + dishByCourse[c];
        }
        result.setScore(entryIndex, a.scoreAt(aIndex) * b.scoreAt(bIndex));
    }

    delete[] dishByCourse;
    delete[] unionScope;
    return result;
}

#endif // MENU_MODEL_HPP
