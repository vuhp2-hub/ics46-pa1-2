// driver.cpp -- PROVIDED for local testing. DO NOT MODIFY (and you do not submit it).
//
// Builds a MenuModel from a .menu file (and optional .chosen file), then exercises the PA 1,2
// functions and prints a readable report. The Gradescope autograder does NOT diff this output --
// it calls your functions directly and checks their RETURN VALUES against the reference. Use this
// program to sanity-check your work locally:
//
//     g++ -std=c++20 -Wall -Wextra driver.cpp -o analyze
//     ./analyze samples/menu3.menu samples/menu3.chosen
#include "MenuModel.hpp"
#include <iostream>
#include <iomanip>

static void printMeal(Meal const& m, int n) {
    for (int c = 0; c < n; ++c) std::cout << (c ? " " : "") << m.dishFor(c);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <file.menu> [file.chosen]\n";
        return 1;
    }
    MenuModel model(argv[1]);
    if (argc >= 3) model.readChosen(argv[2]);

    int n = model.numCourses();
    long long total = model.consistentMealCount();
    std::cout << "courses: " << n << "\n";
    std::cout << "total_meals (ignoring chosen): " << model.totalMealCount() << "\n";
    std::cout << "remaining_meals (given chosen): " << total << "\n";

    // Enumerate every complete meal consistent with the chosen courses (we allocate; fillCompleteMeals fills).
    Meal* meals = new Meal[total];
    fillCompleteMeals(model, meals);
    std::cout << "all_meals (dish per course, in course order):\n";
    for (long long i = 0; i < total; ++i) { std::cout << "  "; printMeal(meals[i], n); std::cout << "\n"; }

    // A sort/search order (reverse course order: last course most significant).
    Course* order = new Course[n];
    for (int k = 0; k < n; ++k) { order[k].courseIdx = n - 1 - k; order[k].numDishes = model.dishes(n - 1 - k); }

    if (total > 0) {
        Meal target = meals[total - 1];                       // a meal we know is present
        // LINEAR search scans the UNSORTED array -- O(N); it only needs Meal::operator== (no sort order).
        std::cout << "linear_search_index (unsorted): "
                  << linearSearchForMeal(meals, static_cast<int>(total), target) << "\n";

        // MERGE SORT (Unit 2) -- sort a COPY of the still-unsorted meals in O(N log N); the fast
        // counterpart to the O(N^2) bubbleSortMeals below. It should reach the SAME order.
        Meal* mergeSorted = new Meal[total];
        for (long long i = 0; i < total; ++i) mergeSorted[i] = meals[i];
        mergeSortMeals(mergeSorted, static_cast<int>(total), order, n);

        // Now SORT in place with bubble, then BINARY-search the sorted array -- O(log N).
        bubbleSortMeals(meals, static_cast<int>(total), order, n);
        std::cout << "sorted_meals (reverse course order):\n";
        for (long long i = 0; i < total; ++i) { std::cout << "  "; printMeal(meals[i], n); std::cout << "\n"; }

        bool mergeMatchesBubble = true;
        for (long long i = 0; i < total; ++i)
            if (mergeSorted[i] != meals[i]) mergeMatchesBubble = false;
        std::cout << "merge_sort_matches_bubble: " << (mergeMatchesBubble ? "yes" : "no") << "\n";
        delete[] mergeSorted;

        // BINARY search on the sorted array -- O(log N).
        std::cout << "binary_search_index (sorted): "
                  << binarySearchForMeal(meals, static_cast<int>(total), target, order, n) << "\n";
    }

    delete[] order;
    delete[] meals;

    // The product of the first two pairing tables (if there are at least two).
    if (model.numTables() >= 2) {
        PairingFactor prod = model.factorAt(0) * model.factorAt(1);
        std::cout << "product(f0,f1) scope:";
        for (int j = 0; j < prod.scopeSize(); ++j) std::cout << " " << prod.courseAt(j);
        std::cout << "\nproduct(f0,f1) scores:" << std::setprecision(17);
        for (long long e = 0; e < prod.numEntries(); ++e) std::cout << " " << prod.scoreAt(e);
        std::cout << "\n";
    }
    return 0;
}
