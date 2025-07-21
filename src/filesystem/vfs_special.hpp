#pragma once
#include <filesystem/base_file.hpp>
#include <memory/stl_sequential.hpp>
#include <errno/errno.h>

namespace Hamster {
class SpecialDriverManager {
public:
    SpecialDriverManager();
    SpecialDriverManager(const SpecialDriverManager &) = delete;
    SpecialDriverManager &operator=(const SpecialDriverManager &) = delete;
    SpecialDriverManager(SpecialDriverManager &&other);
    SpecialDriverManager &operator=(SpecialDriverManager &&other);
    ~SpecialDriverManager();
    int remove_all_drivers();
    int add_driver(BaseSpecialDriver *driver);
    int remove_driver(int id);
    BaseSpecialDriver *get_driver(int id);
private:
    Vector<BaseSpecialDriver *> drivers;
};
} // namespace Hamster
