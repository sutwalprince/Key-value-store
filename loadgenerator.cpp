#include "./httplib.h"
#include <iostream>
#include <bits/stdc++.h>
using namespace std;
#define MAX_THREADS 10

pthread_mutex_t mutex_lock;
void generate_value(int n, string &value)
{
    value.clear();
    for (int i = 0; i < n; ++i)
    {
        char c = 'a' + rand() % 26;
        value += c;
    }
}

vector<double> latencies;

void create_key(httplib::Client &cli)
{
    {

        string key;
        string value;
        int size;

        key = to_string(rand() % 1000);
        size = rand() % 20 + 1;
        generate_value(size, value);

        if (value.size() != size)
        {
            cout << "Value size does not match." << endl;
            return;
        }
        string body = "key=" + key + "&size=" + to_string(size) + "&value=" + value;

        if (auto res = cli.Post("/", body, "application/x-www-form-urlencoded"))
        {
            if (res->status == httplib::StatusCode::OK_200)
            {
                // std::cout << res->body << std::endl;
                if (res->body == "OK")
                {
                    cout << "key-value pair created ." << endl;
                }
                else
                {
                    std::cout << res->body << std::endl;
                }
            }
        }
        else
        {
            auto err = res.error();
            std::cout << "HTTP error: " << httplib::to_string(err) << std::endl;
        }
    }
}

void read_key(httplib::Client &cli)
{
    string key;
    key = to_string(rand() % 1000);
    if (auto res = cli.Get("/" + key))
    {
        if (res->status == httplib::StatusCode::OK_200)
        {
            if (res->body == "-1")
            {
                cout << "Key not found." << endl;
            }
            else
                std::cout << "Value: " << res->body << std::endl;
        }
    }
    else
    {
        auto err = res.error();
        std::cout << "HTTP error: " << httplib::to_string(err) << std::endl;
    }
}

void delete_key(httplib::Client &cli)
{
    string key;
    key = to_string(rand() % 1000);
    if (auto res = cli.Delete("/" + key))
    {
        if (res->status == httplib::StatusCode::OK_200)
        {
            if (res->body == "-1")
            {
                cout << "Key not found." << endl;
            }
            else
                std::cout << "key deleted" << std::endl;
        }
    }
    else
    {
        auto err = res.error();

        std::cout << "HTTP error: " << httplib::to_string(err) << std::endl;
    }
}

void *client_main(void *arg)
{
    httplib::Client cli("localhost", 8080);
    int n = 1000;
    string line;
    int cmd;
    int key = 0;

    while (n--)
    {
        cmd = rand() % 3;
        key = rand() % 1000;

        if (cmd == 0)
        {
            auto start = std::chrono::high_resolution_clock::now();

            create_key(cli);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end - start;
            pthread_mutex_lock(&mutex_lock);
            latencies.push_back(elapsed.count());
            pthread_mutex_unlock(&mutex_lock);
        }
        else if (cmd == 1)
        {
            auto start = std::chrono::high_resolution_clock::now();

            read_key(cli);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end - start;
            pthread_mutex_lock(&mutex_lock);
            latencies.push_back(elapsed.count());
            pthread_mutex_unlock(&mutex_lock);
        }
        else if (cmd == 2)
        {
            auto start = std::chrono::high_resolution_clock::now();

            delete_key(cli);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end - start;
            pthread_mutex_lock(&mutex_lock);
            latencies.push_back(elapsed.count());
            pthread_mutex_unlock(&mutex_lock);
        }
        // sleep(1);
    }

    return 0;
}

int main()
{

    pthread_t threads[MAX_THREADS];

    for (int i = 0; i < MAX_THREADS; ++i)
    {
        pthread_create(&threads[i], nullptr, client_main, nullptr);
    }

    for (int i = 0; i < MAX_THREADS; ++i)
    {
        pthread_join(threads[i], nullptr);
    }
    double total = 0;
    for (double t : latencies)
        total += t;
    double avg = total / latencies.size();
    cout << "Average latency: " << avg << " ms" << endl;
    cout << "Total requests: " << latencies.size() << endl;

    double throughput = (latencies.size() / (total / 1000.0)) ; // roughly
    cout << "Approx throughput: " << throughput << " req/sec" << endl;
    // for (double t : latencies)
    // {
    //     cout << "time (ms): " << t << endl;
    // }

    return 0;
}