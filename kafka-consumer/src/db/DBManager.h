#pragma once
#include <iostream>
#include <pqxx/pqxx>
#include <pqxx/nontransaction>

class DBManager{
    pqxx::connection connection;

public:
    DBManager(std::string host, std::string password, std::string port, std::string DBName, std::string user)
        : connection("host=" + host + " password=" + password + " port=" + port + " dbname=" + DBName + " user=" + user) {}
    
    ~DBManager() { }

    pqxx::result defaultQuery(std::string query){
        pqxx::nontransaction l_work(connection);
        try {
            pqxx::result R{l_work.exec(query)};
            l_work.commit();
            return R;
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            throw;
        }
        throw;
    }

    void transactionQuery(std::string query){
        pqxx::work l_work(connection);
        try {
            pqxx::result R{l_work.exec(query)};
            l_work.commit();
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            throw;
        }
    }
};