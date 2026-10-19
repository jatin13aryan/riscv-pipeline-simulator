
#ifndef REGISTER_FILE_H
#define REGISTER_FILE_H

#include <array>
#include <stdexcept>

class RegisterFile {
private:
    std::array<int, 32> registers{};

public:
    int read(int index) const {
        if (index < 0 || index >= 32) {
            throw std::out_of_range("Invalid register index");
        }

        if (index == 0) {
            return 0;
        }

        return registers[index];
    }

    void write(int index, int value) {
        if (index < 0 || index >= 32) {
            throw std::out_of_range("Invalid register index");
        }

        if (index == 0) {
            return;
        }

        registers[index] = value;
    }
};

#endif
