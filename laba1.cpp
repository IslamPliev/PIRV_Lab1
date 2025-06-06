#include <iostream>      
#include <vector>        
#include <thread>        
#include <mutex>         
#include <atomic>        
#include <chrono>        
#include <boost/thread.hpp>  

//простое ли число
bool is_prime(int chislo) {
    if (chislo < 2) return false;  // единичка - не простое число, поэтому проверяем, что это не она
    for (int i = 2; i*i <= chislo; i++) {
        if (chislo % i == 0) return false;
    }
    return true; 
}

//находим колво простых чисел для одного потока
void naiti_primes(int start, int end, int& count, std::mutex &mtx) {
    int for_one_potok_count = 0;  // для одного потока счетчик
    for (int num = start; num <= end; num++) {
        if (is_prime(num)) {
            for_one_potok_count++;  // Увеличиваем локальный счетчик
        }
    }
    std::lock_guard<std::mutex> lock(mtx);  
    count += for_one_potok_count;  
}


void poisk_primes(int N, int k) { // N - колво элементов, k - на сколько потоков поделим 
    std::vector<boost::thread> threads;  
    std::mutex mtx;
    int total_count = 0;  // общий счетчик 
    int kysok = N / k;  // поделили N элеменов на k частей

    auto start_single = std::chrono::high_resolution_clock::now();
    naiti_primes(1, N, total_count, mtx);
    auto end_single = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_single = end_single - start_single;
    std::cout << "Однопоточное время выполнения: " << time_single.count() << " сек\n";

    auto start_time = std::chrono::high_resolution_clock::now(); 
    for (int i = 0; i < k; i++) {
        int range_start = i * kysok + 1;

        int range_end = (i + 1) * kysok;
        if (i == k - 1) {
            range_end = N; // для последнего потока
        }

        threads.emplace_back(naiti_primes, range_start, range_end, std::ref(total_count), std::ref(mtx));
    }

    for (auto& t : threads) t.join();  

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> raznica = end_time - start_time; //затраченное время

    std::cout << "В отрезке [1, " << N << "] " <<"простых чисел " << total_count << std::endl;
    std::cout << "Время выполнения с " << k << " потоками: " << raznica.count() << " сек" << std::endl;;
}



//2 ЗАДАНИЕ

// С std::atomic<long long>
void with_atomic(const std::vector<int> &arr, int start, int end, std::atomic<long long> &result) {
    long long local_sum = 0;
    for (int i = start; i < end; ++i) {
        local_sum += arr[i];
    }
    result += local_sum;
}

// С std::mutex
void with_mutex(const std::vector<int>& arr, int start, int end, long long& result, std::mutex& mtx) {
    long long local_sum = 0;
    for (int i = start; i < end; ++i) {
        local_sum += arr[i];
    }
    std::lock_guard<std::mutex> lock(mtx);
    result += local_sum;
}

// Без синхронизации (для демонстрации ошибки)
void without(const std::vector<int>& arr, int start, int end, long long& result) {
    for (int i = start; i < end; ++i) {
        result += arr[i]; 
    }
}

void test_array_sum(int N, int k) {
    std::vector<int> arr(N, 1);
    int kysok = N / k;
    std::atomic<long long> atomic_sum(0);
    long long mutex_sum = 0;
    long long no_sync_sum = 0;
    std::mutex mtx;

    //с std::atomic
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> threads;
    for (int i = 0; i < k; ++i) {
        int range_start = i * kysok;
        int range_end = (i + 1) * kysok;
        if (i == k - 1) {
            range_end = N;
        }
        threads.emplace_back(with_atomic, std::ref(arr), range_start, range_end, std::ref(atomic_sum));
    }
    for (auto &t : threads) t.join();
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> raznica = end_time - start_time;
    std::cout << "std::atomic:  " << atomic_sum << " за " << raznica.count() << " сек\n";

    //с std::mutex
    threads.clear();
    start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < k; ++i) {
        int range_start = i * kysok;
        int range_end = (i + 1) * kysok;
        if (i == k - 1) {
            range_end = N;
        }
        threads.emplace_back(with_mutex, std::ref(arr), range_start, range_end, std::ref(mutex_sum), std::ref(mtx));
    }
    for (auto &t : threads) t.join();
    end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> raznica_mutex = end_time - start_time;
    std::cout << "std::mutex:   " << mutex_sum << " за " << raznica_mutex.count() << " сек\n";

    //без синхронизации (для демонстрации ошибки)
    threads.clear();
    start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < k; ++i) {
        int range_start = i * kysok;
        int range_end = (i + 1) * kysok;
        if (i == k - 1) {
            range_end = N;
        }
        threads.emplace_back(without, std::ref(arr), range_start, range_end, std::ref(no_sync_sum));
    }
    for (auto &t : threads) t.join();
    end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> raznica_no_sync = end_time - start_time;
    std::cout << "Без синхронизации (ожидаемая ошибка!): " << no_sync_sum << " за " << raznica_no_sync.count() << " сек\n";
}



    int main() {
        int N = 1000000;
        int array_size = 10000000;
        std::vector<int> thread_counts = { 2, 4, 8 };

        for (int k : thread_counts) {
            std::cout << "Тестирование с " << k << " потоками\n";

            std::cout << "\nПоиск простых чисел\n";
            poisk_primes(N, k);

            std::cout << "\nСуммирование массива\n";
            test_array_sum(array_size, k);
        }
        return 0;
    }