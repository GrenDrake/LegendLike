#include <limits>
#include <iomanip>
#include <sstream>

#include "physfs.h"

#include "actor.h"
#include "board.h"
#include "gamestate.h"
#include "vm.h"
#include "random.h"
#include "textutil.h"
#include "vm_opcode.h"
#include "logger.h"

const static unsigned FILE_ID_NUMBER = 0x004D5654;

std::ostream& operator<<(std::ostream &out, Opcode code) {
    switch(code) {
        case Opcode::exit: out << "Exit"; break;
        case Opcode::stkdup: out << "StkDup"; break;
        case Opcode::stkswap: out << "StkSwap"; break;
        case Opcode::pushw: out << "PushWord"; break;
        case Opcode::readb: out << "ReadByte"; break;
        case Opcode::reads: out << "ReadShort"; break;
        case Opcode::readw: out << "ReadWord"; break;
        case Opcode::storeb: out << "StoreByte"; break;
        case Opcode::stores: out << "StoreShort"; break;
        case Opcode::storew: out << "StoreWord"; break;
        case Opcode::add: out << "Add"; break;
        case Opcode::sub: out << "Sub"; break;
        case Opcode::mul: out << "Mul"; break;
        case Opcode::div: out << "Div"; break;
        case Opcode::mod: out << "Mod"; break;
        case Opcode::inc: out << "Inc"; break;
        case Opcode::dec: out << "Dec"; break;
        case Opcode::call: out << "Call"; break;
        case Opcode::ret: out << "Return"; break;
        case Opcode::jump: out << "Jump"; break;
        case Opcode::jumprel: out << "JumpRel"; break;
        case Opcode::jz: out << "JZ"; break;
        case Opcode::jnz: out << "JNZ"; break;
        case Opcode::jlz: out << "JLZ"; break;
        case Opcode::jgz: out << "JGZ"; break;
        case Opcode::jle: out << "JLE"; break;
        case Opcode::jge: out << "JGE"; break;
        case Opcode::mf_fillbox: out << "mfFillBox"; break;
        case Opcode::mf_drawbox: out << "mfDrawBox"; break;
        case Opcode::mf_settile: out << "mfSetTile"; break;
        case Opcode::mf_horzline: out << "mfHorzLine"; break;
        case Opcode::mf_vertline: out << "mfVertLine"; break;
        case Opcode::mf_addactor: out << "mfAddActor"; break;
        case Opcode::mf_clear: out << "Exit"; break;
        default:
            out << "(OPCODE#" << static_cast<int>(code) << ")";
    }
    return out;
}


VM::VM()
: state(nullptr), mMemory(nullptr), mMemorySize(0), mCurrentPosition(0),
  mImageFile("<memory>"), mIsValid(false)
{ }
VM::~VM() {
    if (mMemory) delete[] mMemory;
}

void VM::setGameState(GameState *newState) {
    state = newState;
}

bool VM::loadFromFile(const std::string &filename, bool requireValid) {
    if (!PHYSFS_exists(filename.c_str())) {
        return false;
    }
    PHYSFS_File *inf = PHYSFS_openRead(filename.c_str());
    auto length = PHYSFS_fileLength(inf);

    mMemory = new char[length];
    PHYSFS_readBytes(inf, mMemory, length);
    PHYSFS_close(inf);

    mMemorySize = length;
    mImageFile = filename;

    if (readWord(0) != FILE_ID_NUMBER) {
        mIsValid = false;
    } else {
        mIsValid = true;
    }
    if (requireValid && !mIsValid) {
        throw VMError(filename + ": Invalid VM image.");
    }

    return true;
}

int VM::getExport(const std::string &name) const {
    if (!mIsValid) return -1;
    int export_count = readWord(exportCountPosition);
    for (int i = 0; i < export_count; ++i) {
        int pos = firstExportPosition + i * exportSize;
        char export_name[20] = { 0 };
        for (int j = 0; j < exportNameSize; ++j, ++pos) {
            export_name[j] = mMemory[pos];
        }
        if (name == export_name) {
            int addr = readWord(pos);
            return addr;
        }
    }
    return -1;
}

bool VM::runFunction(const std::string &name) {
    int address = getExport(name);
    if (address) return run(address);
    return false;
}

int VM::readByte(unsigned address) const {
    if (address >= mMemorySize) throw VMError(mImageFile + ": Tried to read address " + std::to_string(address) + " which is beyond EOF.");
    return mMemory[address];
}

int VM::readShort(unsigned address) const {
    if (address >= mMemorySize) throw VMError(mImageFile + ": Tried to read address " + std::to_string(address) + " which is beyond EOF.");
    unsigned word = 0;
    word |= mMemory[address] & 0xFF;
    word |= (mMemory[address + 1] << 8);
    return word;
}

int VM::readWord(unsigned address) const {
    if (address >= mMemorySize) throw VMError(mImageFile + ": Tried to read address " + std::to_string(address) + " which is beyond EOF.");
    unsigned word = 0;
    word |= mMemory[address] & 0xFF;
    word |= (mMemory[address + 1] << 8) & 0xFF00;
    word |= (mMemory[address + 2] << 16) & 0xFF0000;
    word |= (mMemory[address + 3] << 24) & 0xFF000000;
    return word;
}

std::string VM::readString(unsigned address) const {
    std::string text = &mMemory[address];
    return text;
}

void VM::storeWord(unsigned address, unsigned value) {
    if (address >= mMemorySize) throw VMError(mImageFile + ": Tried to write to address " + std::to_string(address) + " which is beyond EOF.");
    mMemory[address]     = value & 0xFF;
    mMemory[address + 1] = (value >> 8)  & 0xFF;
    mMemory[address + 2] = (value >> 16) & 0xFF;
    mMemory[address + 3] = (value >> 24) & 0xFF;
}

void VM::storeShort(unsigned address, unsigned value) {
    if (address >= mMemorySize) throw VMError(mImageFile + ": Tried to write to address " + std::to_string(address) + " which is beyond EOF.");
    mMemory[address]     = value & 0xFF;
    mMemory[address + 1] = (value >> 8) & 0xFF;
}

void VM::storeByte(unsigned address, unsigned value) {
    if (address >= mMemorySize) throw VMError(mImageFile + ": Tried to write to address " + std::to_string(address) + " which is beyond EOF.");
    mMemory[address] = value & 0xFF;
}

void VM::storeString(unsigned address, const std::string &text, unsigned maxLength) {
    for (std::string::size_type p = 0; p < maxLength; ++p) {
        storeByte(address + p, 0);
    }

    for (std::string::size_type p = 0; p < maxLength - 1 && p < text.size(); ++p) {
        storeByte(address + p, text[p]);
    }
}


unsigned VM::getPosition() const {
    return mCurrentPosition;
}

void VM::setPosition(unsigned address) {
    mCurrentPosition = address;
}

bool VM::validPosition() const {
    return mCurrentPosition < mMemorySize;
}

int VM::readByte() {
    int value = readByte(mCurrentPosition);
    mCurrentPosition += 1;
    return value;
}

int VM::readShort() {
    int value = readShort(mCurrentPosition);
    mCurrentPosition += 2;
    return value;
}

int VM::readWord() {
    int value = readWord(mCurrentPosition);
    mCurrentPosition += 4;
    return value;
}


void VM::push(int value) {
    if (mCallStack.empty()) throw VMError(mImageFile + ": Push on empty callstack.");
    Frame &frame = mCallStack.back();
    if (frame.stackPos >= maxStackSize) throw VMError(mImageFile + ": Stack overflow.");
    frame.stack[frame.stackPos] = value;
    ++frame.stackPos;
}
int VM::stackSize() const {
    if (mCallStack.empty()) throw VMError(mImageFile + ": StackSize on empty callstack.");
    return mCallStack.back().stackPos;
}
int VM::peek(int pos) const {
    if (mCallStack.empty()) throw VMError(mImageFile + ": Peek on empty callstack.");
    const Frame &frame = mCallStack.back();
    return frame.stack[frame.stackPos - pos];
}
int VM::pop() {
    if (mCallStack.empty()) throw VMError(mImageFile + ": Pop on empty callstack.");
    Frame &frame = mCallStack.back();
    if (frame.stackPos == 0) throw VMError(mImageFile + ": Stack underflow.");
    --frame.stackPos;
    return frame.stack[frame.stackPos];
}
void VM::update(int position, int value) {
    if (mCallStack.empty()) throw VMError(mImageFile + ": Update on empty callstack.");
    Frame &frame = mCallStack.back();
    frame.stack[frame.stackPos - position] = value;
}

void VM::minStack(int minimumSize) const {
    if (stackSize() < minimumSize) {
        throw VMError(mImageFile + ": Stack underflow.");
    }
}

bool VM::run(unsigned address) {
    if (!mIsValid) return false;
    if (!mMemory || address >= mMemorySize) {
        return false;
    }

    Board *board = nullptr;
    if (state) board = state->getBoard();
    Opcode opcode;
    unsigned operand;

    std::stringstream currentText;
    unsigned IP = address;
    mCallStack.push_back(Frame(address, 0));
    while (!state->wantsToQuit) {
        if (IP >= mMemorySize) {
            throw VMError(mImageFile
                           + ": Tried to execute instruction at "
                           + std::to_string(IP)
                           + " which is outside memory sized "
                           + std::to_string(mMemorySize)
                           + "\n");
        }

        opcode = static_cast<Opcode>(readByte(IP++));
        switch(opcode) {
            case Opcode::exit:
                return true;

            case Opcode::stkdup:
                minStack(1);
                push(peek(1));
                break;
            case Opcode::stkswap: {
                minStack(2);
                int v1 = peek(1);
                int v2 = peek(2);
                update(1, v2);
                update(2, v1);
                break; }

            case Opcode::pushw:
                operand = readByte(IP++) & 0xFF;
                operand |= ((readByte(IP++)) << 8) & 0xFF00;
                operand |= ((readByte(IP++)) << 16) & 0xFF0000;
                operand |= (readByte(IP++)) << 24;
                push(operand);
                break;

            case Opcode::readb:
                minStack(1);
                push(readByte(pop()));
                break;
            case Opcode::reads:
                minStack(1);
                push(readShort(pop()));
                break;
            case Opcode::readw: {
                minStack(1);
                int addr = pop();
                int value = readWord(addr);
                push(value);
                break; }
            case Opcode::storeb:
                minStack(2);
                operand = pop();
                storeByte(operand, pop());
                break;
            case Opcode::stores:
                minStack(2);
                operand = pop();
                storeShort(operand, pop());
                break;
            case Opcode::storew: {
                minStack(2);
                int addr = pop();
                int value = pop();
                storeWord(addr, value);
                break; }

            case Opcode::add:
                minStack(2);
                update(2, peek(2) + peek(1));
                pop();
                break;
            case Opcode::sub:
                minStack(2);
                update(2, peek(2) - peek(1));
                pop();
                break;
            case Opcode::mul:
                minStack(2);
                update(2, peek(2) * peek(1));
                pop();
                break;
            case Opcode::div:
                minStack(2);
                update(2, peek(2) / peek(1));
                pop();
                break;
            case Opcode::mod:
                minStack(2);
                update(2, peek(2) % peek(1));
                pop();
                break;
            case Opcode::inc:
                minStack(1);
                update(1, peek(1) + 1);
                break;
            case Opcode::dec:
                minStack(1);
                update(1, peek(1) - 1);
                break;

            case Opcode::gets:
                // operand = pop(vm);
                // fgets((char*)&fixed_memory[operand + 1], pop(vm), stdin);
                // operand2 = strlen((char*)&fixed_memory[operand + 1]);
                // fixed_memory[operand] = operand2;
                // fixed_memory[operand + operand2] = 0;
                break;
            case Opcode::saynum:
                currentText << pop();
                break;
            case Opcode::saychar:
                currentText << static_cast<char>(pop());
                break;
            case Opcode::saystr: {
                int stringAddr = pop();
                std::string text = readString(stringAddr);
                currentText << text;
                break; }
            case Opcode::textbox: {
                if (state) {
                    std::string text = currentText.str();
                    currentText.clear();
                    currentText.str("");
                    std::vector<std::string> lines = explode(text, "\n");
                    for (const std::string &s : lines) {
                        state->addMessage(s);
                    }
                }
                break; }

            case Opcode::call: {
                minStack(1);
                Frame frame(pop(), IP);
                mCallStack.push_back(frame);
                IP = frame.functionAddress;
                if (mCallStack.size() > maxCallStack) {
                    throw VMError(mImageFile + ": Exceed maximum call stack size.");
                }
                break; }
            case Opcode::ret: {
                Frame frame = mCallStack.back();
                mCallStack.pop_back();
                IP = frame.returnAddress;
                if (IP == 0) return true;
                break; }

            case Opcode::jnz: {
                minStack(2);
                int value = peek(2);
                int target = peek(1);

                if (value != 0) {
                    IP = target;
                }
                mCallStack.back().stackPos -= 2;
                break; }
            case Opcode::jz: {
                minStack(2);
                int value = peek(2);
                int target = peek(1);

                if (value == 0) {
                    IP = target;
                }
                mCallStack.back().stackPos -= 2;
                break; }
            case Opcode::jlz: {
                minStack(2);
                int value = peek(2);
                int target = peek(1);

                if (value < 0) {
                    IP = target;
                }
                mCallStack.back().stackPos -= 2;
                break; }
            case Opcode::jgz: {
                minStack(2);
                int value = peek(2);
                int target = peek(1);

                if (value > 0) {
                    IP = target;
                }
                mCallStack.back().stackPos -= 2;
                break; }
            case Opcode::jle: {
                minStack(2);
                int value = peek(2);
                int target = peek(1);

                if (value <= 0) {
                    IP = target;
                }
                mCallStack.back().stackPos -= 2;
                break; }
            case Opcode::jge: {
                minStack(2);
                int value = peek(2);
                int target = peek(1);

                if (value >= 0) {
                    IP = target;
                }
                mCallStack.back().stackPos -= 2;
                break; }

            case Opcode::mf_clear: {
                int tile = pop();
                if (board) {
                    board->clearTo(tile);
                }
                break; }
            case Opcode::mf_settile: {
                int tile = pop();
                int x = pop();
                int y = pop();
                if (board) {
                    board->setTile(Point(x, y), tile);
                }
                break; }
            case Opcode::mf_fillrand: {
                int x1 = pop();
                int y1 = pop();
                int x2 = pop();
                int y2 = pop();
                int count = pop();
                std::vector<int> tiles;
                for (int i = 0; i < count; ++i) {
                    tiles.push_back(pop());
                }
                if (board) {
                    for (int x = x1; x <= x2; ++x) {
                        for (int y = y1; y <= y2; ++y) {
                            int tile = tiles[state->coreRNG.next32() % tiles.size()];
                            board->setTile(Point(x, y), tile);
                        }
                    }
                }
                break; }
            case Opcode::mf_fillbox: {
                int tile = pop();
                int x1 = pop();
                int y1 = pop();
                int x2 = pop();
                int y2 = pop();
                if (board) {
                    for (int x = x1; x <= x2; ++x) {
                        for (int y = y1; y <= y2; ++y) {
                            board->setTile(Point(x, y), tile);
                        }
                    }
                }
                break; }
            case Opcode::mf_drawbox: {
                int tile = pop();
                int x1 = pop();
                int y1 = pop();
                int x2 = pop();
                int y2 = pop();
                if (board) {
                    for (int x = x1; x <= x2; ++x) {
                        board->setTile(Point(x, y1), tile);
                        board->setTile(Point(x, y2), tile);
                    }
                    for (int y = y1; y <= y2; ++y) {
                        board->setTile(Point(x1, y), tile);
                        board->setTile(Point(x2, y), tile);
                    }
                }
                break; }
            case Opcode::mf_horzline: {
                int tile = pop();
                int x1 = pop();
                int y = pop();
                int x2 = pop();
                if (board) {
                    for (int x = x1; x <= x2; ++x) {
                        board->setTile(Point(x, y), tile);
                    }
                }
                break; }
            case Opcode::mf_vertline: {
                int tile = pop();
                int x = pop();
                int y1 = pop();
                int y2 = pop();
                if (board) {
                    for (int y = y1; y <= y2; ++y) {
                        board->setTile(Point(x, y), tile);
                    }
                }
                break; }
            case Opcode::mf_addactor: {
                int address = pop();
                addActor(address);
                break; }
            case Opcode::mf_addactors: {
                const int npcSize = 22;
                int baseAddr = pop();
                while (1) {
                    if (readWord(baseAddr) == -1) break;
                    addActor(baseAddr);
                    baseAddr += npcSize;
                }
                break; }
            case Opcode::mf_additem: {
                int itemId = pop();
                int x = pop();
                int y = pop();
                if (board) {
                    if (itemId < 0 || itemId >= static_cast<int>(state->itemDefs.size())) {
                        state->addError("Tried to add unknown item " + std::to_string(itemId) + ".");
                    } else {
                        Item *item = new Item(&state->itemDefs[itemId]);
                        board->addItem(item, Point(x, y));
                    }
                }
                break; }
            case Opcode::mf_addevent: {
                int type = pop();
                int target = pop();
                int x = pop();
                int y = pop();
                if (board) {
                    board->addEvent(Point(x, y), target, type);
                }
                break; }
            case Opcode::mf_fromfile: {
                std::stringstream filename;
                filename << "maps/" << std::setw(4) << std::setfill('0') << board->getInfo().index;
                if (!state->getBoard()->readFromFile(filename.str())) {
                    state->addError("Failed to load map file " + filename.str() + ".");
                }
                break; }
            case Opcode::mf_makemaze: {
                unsigned flags = pop();
                makeMapMaze(state->getBoard(), state->coreRNG, flags);
                break; }
            case Opcode::mf_makefoes: {
                unsigned infoAddr = pop();
                RandomFoeInfo info;
                info.count = readWord(infoAddr);
                infoAddr += 4;
                int type = readWord(infoAddr);
                while (type > 0) {
                    info.typeList.push_back(type);
                    infoAddr += 4;
                    type = readWord(infoAddr);
                }

                mapRandomEnemies(state->getBoard(), state->coreRNG, info);
                break; }

            case Opcode::p_stat: {
                int pos = pop();
                int stat = pop();
                pos += stat; // this is just to silence "unused variable" warnings until this gets reimplemented for real
                push(0);
                break; }
            case Opcode::p_reset: {
                if (board) {
                    state->getPlayer()->reset();
                }
                break; }
            case Opcode::p_damage: {
                int amnt = pop();
                if (board) {
                    state->getPlayer()->takeDamage(amnt);
                }
                break; }
            case Opcode::p_giveitem: {
                int locationNumber = pop();
                if (locationNumber < 0 || locationNumber >= static_cast<int>(state->itemLocations.size())) {
                    state->addError("Invalid location #" + std::to_string(locationNumber));
                    break;
                }
                ItemLocation &locationDef = state->itemLocations[locationNumber];
                if (locationDef.used) {
                    state->addError("Tried to regive location #" + std::to_string(locationNumber));
                    break;
                }
                locationDef.used = true;
                const ItemDef &itemDef = state->itemDefs[locationDef.itemId];
                state->grantItem(itemDef.itemId);
                break; }
            case Opcode::p_claimed: {
                int locationNumber = pop();
                if (locationNumber < 0 || locationNumber >= static_cast<int>(state->itemLocations.size())) {
                    state->addError("Invalid location #" + std::to_string(locationNumber));
                    push(0);
                    break;
                }
                ItemLocation &locationDef = state->itemLocations[locationNumber];
                if (locationDef.used)   push(1);
                else                    push(0);
                break; }
            case Opcode::p_giveitem_imm: {
                int itemId = pop();
                const ItemDef &itemDef = state->itemDefs[itemId];
                state->grantItem(itemDef.itemId);
                break; }
            case Opcode::p_hasitem: {
                int itemId = pop();
                push(state->hasItem(itemId) ? 1 : 0);
                break; }

            case Opcode::warpto: {
                int map = pop();
                int x = pop();
                int y = pop();
                state->warpTo(map, x, y);
                break; }

            default:
                throw VMError(mImageFile
                              + ": Tried to execute unknown instruction "
                              + std::to_string(static_cast<unsigned>(opcode))
                              + " at address "
                              + std::to_string(IP)
                              + ".\n");
        }
    }
    return true;

    return 1;
}

void VM::addActor(int npcAddr) {
    if (!state || !state->getBoard()) return;

    int nameAddr   = readWord (npcAddr);
    int talkFunc   = readWord (npcAddr + 4);
    int special    = readWord (npcAddr + 8);
    unsigned flags = readWord (npcAddr + 12);
    int x          = readShort(npcAddr + 16);
    int y          = readShort(npcAddr + 18);
    int typeId     = readShort(npcAddr + 20);

    std::string name = nameAddr ? readString(nameAddr) : "";
    Actor *actor = new Actor(typeId);
    state->getBoard()->addActor(actor, Point(x, y));
    actor->name = name;
    actor->talkFunc = talkFunc;
    actor->talkArg = special;
    if (flags & 0x01) actor->hasProperName = true;
    actor->reset();
}
