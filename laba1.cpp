#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <boost/thread.hpp>  

// Функция проверки числа на простоту
bool is_prime(int n) {
    if (n < 2) return false;
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0) return false;
    }
    return true;
}

// Поиск простых чисел в диапазоне (с использованием мьютекса)
void find_primes(int start, int end, int& count, std::mutex& mtx) {
    int local_count = 0;
    for (int num = start; num <= end; ++num) {
        if (is_prime(num)) {
            ++local_count;
        }
    }
    std::lock_guard<std::mutex> lock(mtx);
    count += local_count;
}

// Тестирование поиска простых чисел
void test_prime_search(int N, int num_threads) {
    std::vector<std::thread> threads;
    std::mutex mtx;
    int total_primes = 0;
    
    // Однопоточный режим
    auto start_single = std::chrono::high_resolution_clock::now();
    find_primes(1, N, total_primes, mtx);
    auto end_single = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> single_time = end_single - start_single;
    
    total_primes = 0; // Сброс счетчика
    
    // Многопоточный режим
    int chunk_size = N / num_threads;
    auto start_multi = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_threads; ++i) {
        int start = i * chunk_size + 1;
        int end = (i == num_threads - 1) ? N : (i + 1) * chunk_size;
        threads.emplace_back(find_primes, start, end, std::ref(total_primes), std::ref(mtx));
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end_multi = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> multi_time = end_multi - start_multi;
    
    std::cout << "Простых чисел в [1, " << N << "]: " << total_primes << "\n";
    std::cout << "Однопоточное время: " << single_time.count() << " сек\n";
    std::cout << "Многопоточное время (" << num_threads << " потоков): " 
              << multi_time.count() << " сек\n\n";
}

// Суммирование массива с atomic
void sum_with_atomic(const std::vector<int>& arr, int start, int end, 
                    std::atomic<long long>& result) {
    long long local_sum = 0;
    for (int i = start; i < end; ++i) {
        local_sum += arr[i];
    }
    result += local_sum;
}

// Суммирование массива с mutex
void sum_with_mutex(const std::vector<int>& arr, int start, int end, 
                   long long& result, std::mutex& mtx) {
    long long local_sum = 0;
    for (int i = start; i < end; ++i) {
        local_sum += arr[i];
    }
    std::lock_guard<std::mutex> lock(mtx);
    result += local_sum;
}

// Тестирование суммирования массива
void test_array_sum(int N, int num_threads) {
    std::vector<int> arr(N, 1); // Массив из единиц
    
    // Atomic вариант
    std::atomic<long long> atomic_sum(0);
    std::vector<std::thread> threads;
    int chunk_size = N / num_threads;
    
    auto start_atomic = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_threads; ++i) {
        int start = i * chunk_size;
        int end = (i == num_threads - 1) ? N : (i + 1) * chunk_size;
        threads.emplace_back(sum_with_atomic, std::ref(arr), start, end, std::ref(atomic_sum));
    }
    
    for (auto& t : threads) {
        t.join();
    }
    auto end_atomic = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> atomic_time = end_atomic - start_atomic;
    
    // Mutex вариант
    threads.clear();
    long long mutex_sum = 0;
    std::mutex mtx;
    
    auto start_mutex = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_threads; ++i) {
        int start = i * chunk_size;
        int end = (i == num_threads - 1) ? N : (i + 1) * chunk_size;
        threads.emplace_back(sum_with_mutex, std::ref(arr), start, end, 
                           std::ref(mutex_sum), std::ref(mtx));
    }
    
    for (auto& t : threads) {
        t.join();
    }
    auto end_mutex = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> mutex_time = end_mutex - start_mutex;
    
    std::cout << "Сумма массива (atomic): " << atomic_sum 
              << ", время: " << atomic_time.count() << " сек\n";
    std::cout << "Сумма массива (mutex): " << mutex_sum 
              << ", время: " << mutex_time.count() << " сек\n\n";
}

int main() {
    const int N_primes = 1000000;
    const int N_array = 10000000;
    std::vector<int> thread_counts = {2, 4, 8};
    
    for (int threads : thread_counts) {
        std::cout << "=== Тестирование с " << threads << " потоками ===\n";
        
        std::cout << "\n[Поиск простых чисел]\n";
        test_prime_search(N_primes, threads);
        
        std::cout << "[Суммирование массива]\n";
        test_array_sum(N_array, threads);
    }
    
    return 0;
}