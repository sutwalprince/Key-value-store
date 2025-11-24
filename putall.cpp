#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <pthread.h>
#include <chrono>
#include <atomic>
#include <signal.h>
#include "httplib.h"

using namespace std;

pthread_mutex_t mutex_lock;


vector<double> threadTimes;
atomic<long> totalRequests(0);
atomic<long> failedRequests(0);
atomic<long> totalLatencyMicro(0);   
atomic<bool> stopNow(false);

void handle_alarm(int sig) {
    stopNow = true;
}

void generate_value(int n, string &value) {
    value.clear();
    for (int i = 0; i < n; i++)
        value += char('a' + rand() % 26);
}

bool create_key(httplib::Client &cli, int key_int, long &latencyMicro) {
    string key = to_string(key_int);
    int size = (rand() % 400) + 1;
    string value;
    generate_value(size, value);

    string body = "key=" + key  + "&value=" + value;

    auto req_start = chrono::high_resolution_clock::now();  

    bool success = false;
    if (auto res = cli.Post("/", body, "application/x-www-form-urlencoded")) {
        if (res->status == 200) success = true;
    }

    auto req_end = chrono::high_resolution_clock::now();    
    latencyMicro = chrono::duration_cast<chrono::microseconds>(req_end - req_start).count();

    return success;
}

void* client_thread(void* arg) {
    httplib::Client cli("localhost", 8080);
    long localRequests = 0;
    long localFailed = 0;

    int key = *(int*)arg * 1000000;
    long localLatency = 0;         

    auto start = chrono::high_resolution_clock::now();

    while (!stopNow) {
        long reqLatency;
        if (create_key(cli, key, reqLatency)) {
            localRequests++;
            localLatency += reqLatency;
        } else localFailed++;

        key++;
        // cout << "Sent key: " << key << "\n";  
    }

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> elapsed = end - start;

    pthread_mutex_lock(&mutex_lock);
    totalRequests += localRequests;
    failedRequests += localFailed;
    totalLatencyMicro += localLatency;     
    threadTimes.push_back(elapsed.count());
    pthread_mutex_unlock(&mutex_lock);

    return nullptr;
}

int main(int argc, char** argv) {
    pthread_mutex_init(&mutex_lock, NULL);
    int  DURATION_SECONDS = 300 ;
    int NUM_THREADS = 8;
    if (argc > 1) NUM_THREADS = stoi(argv[1]);
    if(argc > 2) DURATION_SECONDS = stoi(argv[2]);

    cout << " Starting load test with " << NUM_THREADS << " threads for " 
         << DURATION_SECONDS << " seconds...\n";

    signal(SIGALRM, handle_alarm);
    alarm(DURATION_SECONDS);

    vector<pthread_t> threads(NUM_THREADS);
    vector<int> ids(NUM_THREADS);

    auto globalStart = chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], nullptr, client_thread, (void*)&ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], nullptr);

    auto globalEnd = chrono::high_resolution_clock::now();
    chrono::duration<double> total = globalEnd - globalStart;

    cout << "Time Elapsed           : " << total.count() << " sec\n";
    cout << "Total Requests Sent    : " << totalRequests << "\n";
    cout << "Failed Requests        : " << failedRequests 
         << "  (" << (100.0 * failedRequests / totalRequests) << "%)\n";
    cout << "Throughput             : " << totalRequests / total.count() << " req/sec\n";

    
    double avgLatencyMs = (totalLatencyMicro.load() / 1000.0) / totalRequests.load();
    cout << "Average Latency        : " << avgLatencyMs << " ms\n";

    double sum = 0;
    for (double t : threadTimes) sum += t;
    double avg = sum / threadTimes.size();
    cout << "Avg Thread Execution Time: " << avg << " ms\n";

    pthread_mutex_destroy(&mutex_lock);
    return 0;
}




































// #include <iostream>
// #include <string>
// #include <vector>
// #include <algorithm>
// #include <pthread.h>
// #include <chrono>
// #include <atomic>
// #include <signal.h>
// #include "httplib.h"

// using namespace std;

// // ================== CONFIGURATION ================== //
//      // run test for 120 seconds

// // =================================================== //

// pthread_mutex_t mutex_lock;

// // Global shared counters
// vector<double> threadTimes;
// atomic<long> totalRequests(0);
// atomic<long> failedRequests(0);
// atomic<bool> stopNow(false);


// void handle_alarm(int sig) {
//     stopNow = true;
// }


// void generate_value(int n, string &value) {
//     value.clear();
//     for (int i = 0; i < n; i++)
//         value += char('a' + rand() % 26);
// }


// bool create_key(httplib::Client &cli, int key_int) {
//     string key = to_string(key_int);
//     int size = (rand() % 20) + 1;
//     string value;
//     generate_value(size, value);

//     string body = "key=" + key  + "&value=" + value;

//     if (auto res = cli.Post("/", body, "application/x-www-form-urlencoded")) {
//         if (res->status == 200) return true;
//     }
//     return false;  
// }


// void* client_thread(void* arg) {
//     httplib::Client cli("localhost", 8080);
//     long localRequests = 0;
//     long localFailed = 0;

//     int key = *(int*)arg * 1000000;
    

//     auto start = chrono::high_resolution_clock::now();

//     while (!stopNow) {
//         if (create_key(cli, key)) localRequests++;
//         else localFailed++;
//         key++;
//         cout << "Sent key: " << key << "\n";
//     }

//     auto end = chrono::high_resolution_clock::now();
//     chrono::duration<double, milli> elapsed = end - start;

    
//     pthread_mutex_lock(&mutex_lock);
//     totalRequests += localRequests;
//     failedRequests += localFailed;
//     threadTimes.push_back(elapsed.count());
//     pthread_mutex_unlock(&mutex_lock);

//     return nullptr;
// }


// int main(int argc, char** argv) {
//     pthread_mutex_init(&mutex_lock, NULL);
//     int  DURATION_SECONDS = 300 ;
//     int NUM_THREADS = 8;
//     if (argc > 1) NUM_THREADS = stoi(argv[1]);
//     if(argc > 2) DURATION_SECONDS = stoi(argv[2]);
    

//     cout << " Starting load test with " << NUM_THREADS << " threads for " 
//          << DURATION_SECONDS << " seconds...\n";

    
//     signal(SIGALRM, handle_alarm);
//     alarm(DURATION_SECONDS);      

//     vector<pthread_t> threads(NUM_THREADS);
//     vector<int> ids(NUM_THREADS);

//     auto globalStart = chrono::high_resolution_clock::now();

//     for (int i = 0; i < NUM_THREADS; i++) {
//         ids[i] = i;
//         pthread_create(&threads[i], nullptr, client_thread, (void*)&ids[i]);
//     }

//     for (int i = 0; i < NUM_THREADS; i++)
//         pthread_join(threads[i], nullptr);

//     auto globalEnd = chrono::high_resolution_clock::now();
//     chrono::duration<double> total = globalEnd - globalStart;

//     cout << "\n===========  FINAL REPORT  ===========\n";
//     cout << "Time Elapsed           : " << total.count() << " sec\n";
//     cout << "Total Requests Sent    : " << totalRequests << "\n";
//     cout << "Failed Requests        : " << failedRequests 
//          << "  (" << (100.0 * failedRequests / totalRequests) << "%)\n";
//     cout << "Throughput             : " << totalRequests / total.count() << " req/sec\n";

//     // Latency per thread (avg)
//     double sum = 0;
//     for (double t : threadTimes) sum += t;
//     double avg = sum / threadTimes.size();

//     cout << "Avg Thread Execution Time: " << avg << " ms\n";
//     cout << "===========================================\n";

//     pthread_mutex_destroy(&mutex_lock);
//     return 0;
// }
