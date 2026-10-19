
#ifndef MEMORY_H
#define MEMORY_H

#include <vector>
#include <stdexcept>

class Memory {
private:
    std::vector<int> words;

public:
    explicit Memory(int wordCount = 256)
        : words(wordCount, 0) {
        if (wordCount <= 0) {
            throw std::invalid_argument(
                "Memory size must be positive"
            );
        }
    }

    int loadWord(int byteAddress) const {
        checkAddress(byteAddress);
        return words[byteAddress / 4];
    }

    void storeWord(int byteAddress, int value) {
        checkAddress(byteAddress);
        words[byteAddress / 4] = value;
    }

private:
    void checkAddress(int byteAddress) const {
        if (byteAddress < 0 ||
            byteAddress % 4 != 0 ||
            byteAddress / 4 >= static_cast<int>(words.size())) {
            throw std::out_of_range(
                "Invalid or unaligned memory address"
            );
        }
    }
};

#endif
