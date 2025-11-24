#include "pch.h"
using namespace std;


pthread_mutex_t mutex_lock;
void generate_value(int n, string &value)
{
    value.clear();
    for (int i = 0; i < n; i++)
    {
        char c = 'a' + rand() % 26;
        value += c;
    }
}

vector<double> timeArr;

void create_key(httplib::Client &cli)
{

    string key;
    string value;
    int size;

    key = to_string(rand() % 100000);
    size = (rand() % 20) + 1;
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
    int cmd;
    int key = 0;

    auto start = std::chrono::high_resolution_clock::now();
    while (n--)
    {
        cmd = rand() % 3;
        key = rand() % 1000000;

        if (cmd == 0)
        {

            create_key(cli);
        }
        else if (cmd == 1)
        {

            read_key(cli); 
        }
        else if (cmd == 2)
        {
            delete_key(cli);
        }

        // sleep(1);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    pthread_mutex_lock(&mutex_lock);
    timeArr.push_back(elapsed.count());
    pthread_mutex_unlock(&mutex_lock);

    return 0;
}

int main(int argc, char **argv)
{

    int MAX_THREADS = 10;
    if(argc > 1)
    {
        int MAX_THREADS = stoi(argv[1]);
    }
    pthread_t threads[MAX_THREADS];
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < MAX_THREADS; ++i)
    {
        pthread_create(&threads[i], nullptr, client_main, nullptr);
    }

    for (int i = 0; i < MAX_THREADS; ++i)
    {
        pthread_join(threads[i], nullptr);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    cout << "Total time (ms): " << elapsed.count() << endl;
    for (double t : timeArr)
    {
        cout << "time (ms): " << t << endl;
    }

    return 0;
}