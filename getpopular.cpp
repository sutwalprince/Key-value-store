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

// Global shared counters
vector<double> threadTimes;
atomic<long> totalRequests(0);
atomic<long> failedRequests(0);
atomic<long> totalLatencyMicro(0);  
atomic<bool> stopNow(false);

void handle_alarm(int sig)
{
    stopNow = true;
}

bool read_key(httplib::Client &cli, int key_int, long &latencyMicro)
{
    string key = to_string(key_int);

    auto t1 = chrono::high_resolution_clock::now();
    bool success = false;

    if (auto res = cli.Get("/" + key))
        if (res->status == 200)
            {   
                success = true;
            }
    auto t2 = chrono::high_resolution_clock::now();
    latencyMicro = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();

    return success;
}

void *client_thread(void *arg)
{
    httplib::Client cli("localhost", 8080);
    long localRequests = 0;
    long localFailed = 0;
    long localLatency = 0;   

    auto start = chrono::high_resolution_clock::now();

    while (!stopNow)
    {
        int key = rand() % 10;

        long reqLatency;
        if (read_key(cli, key, reqLatency))
        {
            localRequests++;
            localLatency += reqLatency;  
        }
        else
            localFailed++;

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

int main(int argc, char **argv)
{
    pthread_mutex_init(&mutex_lock, NULL);
    int DURATION_SECONDS = 300;
    int NUM_THREADS = 8;
    if (argc > 1)
        NUM_THREADS = stoi(argv[1]);
    if (argc > 2)
        DURATION_SECONDS = stoi(argv[2]);

    cout << " Starting load test with " << NUM_THREADS << " threads for "
         << DURATION_SECONDS << " seconds...\n";

    signal(SIGALRM, handle_alarm);
    alarm(DURATION_SECONDS);

    vector<pthread_t> threads(NUM_THREADS);
    vector<int> ids(NUM_THREADS);

    auto globalStart = chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_THREADS; i++)
    {
        ids[i] = i;
        pthread_create(&threads[i], nullptr, client_thread, (void *)&ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], nullptr);

    auto globalEnd = chrono::high_resolution_clock::now();
    chrono::duration<double> total = globalEnd - globalStart;

    cout << "Time Elapsed              : " << total.count() << " sec\n";
    cout << "Total Requests Sent       : " << totalRequests << "\n";
    cout << "Failed Requests           : " << failedRequests
         << "  (" << (100.0 * failedRequests / totalRequests) << "%)\n";
    cout << "Throughput                : " << totalRequests / total.count() << " req/sec\n";

    
    double avgLatencyMs = (totalLatencyMicro.load() / 1000.0) / totalRequests.load();
    cout << "Average Latency           : " << avgLatencyMs << " ms\n";

    double sum = 0;
    for (double t : threadTimes)
        sum += t;
    double avg = sum / threadTimes.size();
    cout << "Avg Thread Execution Time: " << avg << " ms\n";

    pthread_mutex_destroy(&mutex_lock);
    return 0;
}
