#ifndef VM_H
#define VM_H

#include <array>
#include <stdexcept>
#include <string>
#include <vector>

class GameState;

class VMError : public std::runtime_error {
public:
    VMError(const std::string &msg)
    : std::runtime_error(msg)
    { }
};

class VM {
public:
    static const int exportCountPosition = 48;
    static const int firstExportPosition = 52;
    static const int exportNameSize = 16;
    static const int exportSize = 20;
    static const int maxStackSize = 128;
    static const int maxCallStack = 64;

    VM();
    ~VM();

    void setGameState(GameState *newState);
    bool loadFromFile(const std::string &filename, bool requireValid);

    int getExport(const std::string &name) const;
    bool runFunction(const std::string &name);
    bool run(unsigned start_address);

    int readByte(unsigned address) const;
    int readShort(unsigned address) const;
    int readWord(unsigned address) const;
    std::string readString(unsigned address) const;
    void storeWord(unsigned address, unsigned value);
    void storeShort(unsigned address, unsigned value);
    void storeByte(unsigned address, unsigned value);
    void storeString(unsigned address, const std::string &text, unsigned maxLength);

    unsigned getPosition() const;
    void setPosition(unsigned address);
    int rewind(bool skipHeader);
    bool validPosition() const;
    int readByte();
    int readShort();
    int readWord();



private:
    struct Frame {
        Frame(unsigned address, unsigned returnTo)
        : functionAddress(address), returnAddress(returnTo), stackPos(0)
        { }

        unsigned functionAddress;
        unsigned returnAddress;

        unsigned stackPos;
        std::array<int, maxStackSize> stack;
    };

    void push(int value);
    int stackSize() const;
    int peek(int position) const;
    int pop();
    void update(int position, int value);
    void minStack(int minimumSize) const;

    void addActor(int npcAddr);

    GameState *state;
    char *mMemory;
    unsigned long long mMemorySize;
    std::vector<Frame> mCallStack;
    unsigned mCurrentPosition;
    std::string mImageFile;
    bool mIsValid;
};

#endif



