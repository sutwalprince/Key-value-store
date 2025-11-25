#include <iostream>
#include <string>
#include <list>
#include <unordered_map>
#include <mutex>
#include "httplib.h"
#include "/usr/include/postgresql/libpq-fe.h"
// #define CACHE_CAPACITY 10
#define MAX_THREADS 8

pthread_mutex_t lock;

class LRUCache
{
    int capacity;
    std::list<int> cache;
    std::unordered_map<int, std::pair<std::string, std::list<int>::iterator>> kv_store;

public:
    LRUCache(int cap)
    {
        capacity = cap;
    }

    bool exists(const int &key)
    {
        return kv_store.find(key) != kv_store.end();
    }

    void put(const int &key, const std::string &value)
    {
        if (exists(key))
        {
            cache.erase(kv_store[key].second); // move to front
        }
        else if (cache.size() >= capacity)
        {
            int last = cache.back();
            kv_store.erase(last);
            cache.pop_back();
        }
        cache.push_front(key);
        kv_store[key] = {value, cache.begin()};
    }

    bool get(const int &key, std::string &value)
    {
        if (!exists(key))
            return false;
        value = kv_store[key].first + " (from cache)";
        cache.erase(kv_store[key].second);
        cache.push_front(key);
        kv_store[key].second = cache.begin();
        return true;
    }

    void delete_key(const int &key)
    {
        if (!exists(key))
            return;
        cache.erase(kv_store[key].second);
        kv_store.erase(key);
    }

    void print()
    {
        std::cout << "Cache : ";
        for (auto &p : cache)
            std::cout << p << ":" << kv_store[p].first << std::endl;
        std::cout << "\n";
    }
};

thread_local PGconn *thread_conn = nullptr;

PGconn *get_thread_connection()
{
    if (thread_conn == nullptr)
    {
        thread_conn = PQconnectdb(
            "host=localhost port=5432 dbname=kvstore user=kvuser password=1234");

        if (PQstatus(thread_conn) != CONNECTION_OK)
        {
            std::cerr << "[DB] Connection failed in thread: "
                      << PQerrorMessage(thread_conn);
            PQfinish(thread_conn);
            thread_conn = nullptr;
            return nullptr;
        }
        std::cout << "[DB] Thread created new PGconn thread id : " << std::this_thread::get_id() << "\n";
    }
    return thread_conn;
}

int save_key_to_db(PGconn *conn, const int key, const std::string &value)
{
    std::string key_str = std::to_string(key);
    const char *param_values[] = {key_str.c_str(), value.c_str()};

    PGresult *r = PQexecParams(conn,
                               "INSERT INTO kv_store (key, value) VALUES ($1, $2) ;",
                               2,
                               NULL,
                               param_values,
                               NULL, NULL, 0);
    if (PQresultStatus(r) != PGRES_COMMAND_OK)
    {
        // std::cerr << "INSERT failed: " << PQerrorMessage(conn);
        PQclear(r);
        return 0;
    }
    PQclear(r) ;
    return 1;
}

void all_keys_in_db(PGconn *conn)
{
    PGresult *r = PQexecParams(conn,
                               "SELECT key, value FROM kv_store;",
                               0,
                               NULL,
                               NULL,
                               NULL, NULL, 0);
    if (PQresultStatus(r) != PGRES_TUPLES_OK)
    {
        std::cerr << "SELECT failed: " << PQerrorMessage(conn);
        PQclear(r);
        return;
    }

    int nrows = PQntuples(r);
    for (int i = 0; i < nrows; i++)
    {
        char *key_str = PQgetvalue(r, i, 0);
        char *val_str = PQgetvalue(r, i, 1);
        std::cout << "Key: " << key_str << ", Value: " << val_str << std::endl;
    }

    PQclear(r);
}

int search_key_in_db(PGconn *conn, const int key, std::string &value)
{
    std::string key_str = std::to_string(key);
    const char *param_values[] = {key_str.c_str()};

    PGresult *r = PQexecParams(conn,
                               "SELECT value FROM kv_store WHERE key = $1;",
                               1,
                               NULL,
                               param_values,
                               NULL, NULL, 0);
    if (PQresultStatus(r) != PGRES_TUPLES_OK)
    {
        std::cerr << "SELECT failed: " << PQerrorMessage(conn);
        PQclear(r);
        return -1;
    }

    if (PQntuples(r) == 0)
    {
        PQclear(r);
        return 0;
    }

    char *val_cstr = PQgetvalue(r, 0, 0);
    value = std::string(val_cstr);

    PQclear(r);
    return 1;
}

bool key_exists_in_db(PGconn *conn, const int key)
{
    std::string key_str = std::to_string(key);
    const char *param_values[] = {key_str.c_str()};

    PGresult *r = PQexecParams(conn,
                               "SELECT 1 FROM kv_store WHERE key = $1;",
                               1,
                               NULL,
                               param_values,
                               NULL, NULL, 0);
    if (PQresultStatus(r) != PGRES_TUPLES_OK)
    {
        std::cerr << "SELECT failed: " << PQerrorMessage(conn);
        PQclear(r);
        return false;
    }

    bool exists = (PQntuples(r) > 0);
    PQclear(r);
    return exists;
}

int delete_key_from_db(PGconn *conn, int key)
{
    std::string key_str = std::to_string(key);
    const char *param_values[] = {key_str.c_str()};

    PGresult *r = PQexecParams(conn,
                               "DELETE FROM kv_store WHERE key = $1;",
                               1,
                               NULL,
                               param_values,
                               NULL, NULL, 0);
    if (PQresultStatus(r) != PGRES_COMMAND_OK)
    {
        std::cerr << "DELETE failed: " << PQerrorMessage(conn);
        PQclear(r);
        return 0;
    }
    PQclear(r);
    return 1;
}

void create_table_if_not_exists(PGconn *conn)
{
    PGresult *r = PQexecParams(conn,
                               "CREATE TABLE IF NOT EXISTS kv_store (key INTEGER PRIMARY KEY, value TEXT NOT NULL);",
                               0,
                               NULL,
                               NULL,
                               NULL, NULL, 0);
    if (PQresultStatus(r) != PGRES_COMMAND_OK)
    {
        std::cerr << "CREATE TABLE failed: " << PQerrorMessage(conn);
        PQclear(r);
    }
    // PQclear(r);
    
    // r = PQexecParams(conn,
    //                  "TRUNCATE TABLE kv_store;",
    //                  0,
    //                  NULL,
    //                  NULL,
    //                  NULL, NULL, 0);
    // if (PQresultStatus(r) != PGRES_COMMAND_OK)
    // {
    //     std::cerr << "TRUNCATE TABLE failed: " << PQerrorMessage(conn);
    // }
    PQclear(r);
}

PGconn *connect_to_db()
{
    PGconn *conn = PQconnectdb("host=localhost port=5432 dbname=kvstore user=kvuser password=1234");

    if (PQstatus(conn) != CONNECTION_OK)
    {
        std::cerr << "DB connection failed: " << PQerrorMessage(conn);
        PQfinish(conn);
        return nullptr;
    }

    std::cout << "Connected to DB\n";
    return conn;
}

int main(int argc, char *argv[])
{
    pthread_mutex_init(&lock, NULL);
    int CACHE_CAPACITY = argc > 1 ? std::stoi(argv[1]) : 100;

    PGconn *conn = connect_to_db();
    if (!conn)
    {
        return 1;
    }
    create_table_if_not_exists(conn);
    LRUCache kv_cache(CACHE_CAPACITY);

    using namespace httplib;

    Server svr;

    svr.Get("/hi", [](const Request &req, Response &res)
            { res.set_content("Hello World!", "text/plain"); });

    svr.Post("/", [&kv_cache](const Request &req, Response &res)
             {
                PGconn *conn = get_thread_connection();
    if (!conn)
    {
        res.status = 500;
        res.set_content("DB connection failed", "text/plain");
        return;
    }
    int key = std::stoi(req.get_param_value("key"));

    std::string value = req.get_param_value("value");
    
    if (save_key_to_db(conn, key, value) != 1)
    {
        res.set_content("Already in database.", "text/plain");
        return;
    }else{
        pthread_mutex_lock(&lock);
        kv_cache.put(key, value);
        pthread_mutex_unlock(&lock);
    }
  
    res.set_content("OK", "text/plain"); });

    svr.Get(R"(/(\d+))", [&kv_cache](const Request &req, Response &res)
            {
    PGconn *conn = get_thread_connection();
    if (!conn)
    {
        res.status = 500;
        res.set_content("DB connection failed", "text/plain");
        return;
    }
    auto numbers = req.matches[1];
    int key = std::stoi(numbers);
    std::string value;
    pthread_mutex_lock(&lock);
    if (kv_cache.get(key, value) == false)
    {
    pthread_mutex_unlock(&lock);

        if (search_key_in_db(conn, key, value) != 1)
        {
            pthread_mutex_unlock(&lock);
            res.set_content("-1", "text/plain");
            return;
        }
        pthread_mutex_lock(&lock);
        kv_cache.put(key, value);
    }
    pthread_mutex_unlock(&lock);
    res.set_content(value, "text/plain"); });

    svr.Delete(R"(/(\d+))", [&kv_cache](const Request &req, Response &res)
               {
    PGconn *conn = get_thread_connection();
    if (!conn)
    {
        res.status = 500;
        res.set_content("DB connection failed", "text/plain");
        return;
    }
    auto numbers = req.matches[1];
    int key = std::stoi(numbers);
    pthread_mutex_lock(&lock);
    if (!kv_cache.exists(key))
    {
        if (!key_exists_in_db(conn, key))
        {
            pthread_mutex_unlock(&lock);
            res.set_content("-1", "text/plain");
            return;
        }
    }
    kv_cache.delete_key(key) ;
    if (delete_key_from_db(conn, key) != 1)
    {
        pthread_mutex_unlock(&lock);
        res.set_content("Failed to delete from database", "text/plain");
        return;
    }
    // all_keys_in_db(conn);
    // kv_cache.print();
    pthread_mutex_unlock(&lock);
    res.set_content("OK", "text/plain"); });

    svr.Get("/stop", [&](const Request &, Response &res)
            {
    PQfinish(conn);
    res.set_content("Server stopping", "text/plain");
    svr.stop();
    PQfinish(conn);
    pthread_mutex_destroy(&lock); });

    svr.new_task_queue = []
    { return new ThreadPool(MAX_THREADS); };

    // if (svr.listen("localhost", 8080))
    if (svr.listen("0.0.0.0", 8080))
    {
        std::cout << "server listening on port 8080" << std::endl;
    }
    return 0;
}
