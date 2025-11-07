#include "./httplib.h"
#include <iostream>
#include <bits/stdc++.h>
using namespace std;

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
            cout << "Value size does not match ." << endl;
            return;
        }
        string body = "key=" + key + "&size=" + to_string(size) + "&value=" + value;

        if (auto res = cli.Post("/", body, "application/x-www-form-urlencoded"))
        {
            if (res->status == httplib::StatusCode::OK_200)
            {
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

void delete_key(httplib::Client &cli, istringstream &iss)
{
    string key;
    iss >> key;
    if (key.empty())
    {
        cout << "No key provided." << endl;
        return;
    }
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

int main(void)
{
    httplib::Client cli("localhost", 8080);

    string line;
    while (1)
    {
        cout << "$ ";
        getline(cin, line);

        if (line.empty())
        {
        }
        else
        {
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
    }

    return 0;
}