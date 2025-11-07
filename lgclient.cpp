#include "./httplib.h"
#include <iostream>
#include <bits/stdc++.h>
using namespace std;

void generate_value(int n, string &value)
{
    value.clear();
    for (int i = 0; i < n; ++i)
    {
        char c = 'a' + rand() % 26;
        value += c;
    }
}

void create_key(httplib::Client &cli, istringstream &iss)
{
    {
        string arg;
        string key;
        string value;
        int size;
        if (iss >> arg)
        {
            key = arg;
        }
        if (iss >> arg)
        {
            size = stoi(arg);
        }
        if (getline(iss, value))
        {
            // Remove leading whitespace
            value.erase(0, value.find_first_not_of(" \t"));
        }
        if (value.size() != size)
        {
            cout << "Value size does not match the specified size." << endl;
            return;
        }
        string body = "key=" + key + "&size=" + to_string(size) + "&value=" + value;

        if (auto res = cli.Post("/create", body, "application/x-www-form-urlencoded"))
        {
            if (res->status == httplib::StatusCode::OK_200)
            {
                std::cout << res->body << std::endl;
                if (res->body == "OK")
                {
                    cout << "key-value pair created successfully." << endl;
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

void read_key(httplib::Client &cli, istringstream &iss)
{
    string key;
    iss >> key;
    if (key.empty())
    {
        cout << "No key provided." << endl;
        return;
    }
    cout << "Reading key: " << key << endl;
    if (auto res = cli.Get("/read/" + key))
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

void delete_key(httplib::Client &cli, istringstream &iss)
{
    string key;
    iss >> key;
    if (key.empty())
    {
        cout << "No key provided." << endl;
        return;
    }
    cout << "deleting key: " << key << endl;
    if (auto res = cli.Delete("/delete/" + key))
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

int main(void)
{
    httplib::Client cli("localhost", 8080);
    int n = 1000;
    string line;
    int x ;
    int key ;
    while (n--)
    {
        x = 0;
        key = rand() % 100;
        if (x == 0)
        {
            line = "create ";
            int size = rand() % 20 + 1;
            string value;
            generate_value(size, value);
            line += to_string(key) + " " + to_string(size) + " " + value;
        }
        else if (x == 1)
        {
            line = "read ";
            int key = rand() % 100;
            line += to_string(key);
        }
        else
        {
            line = "delete ";
            int key = rand() % 100;
            line += to_string(key);
        }

        istringstream iss(line);
        string cmd;
        string arg;


        if (iss >> arg)
        {
            cmd = arg;
        }

        string remaining;

        if (cmd == "exit" || cmd == "quit")
        {
            break;
        }
        else if (cmd == "create")
        {
            create_key(cli, iss);
        }
        else if (cmd == "read")
        {
            read_key(cli, iss);
        }
        else if (cmd == "delete")
        {
            delete_key(cli, iss);
        }
    }
    return 0;
}

