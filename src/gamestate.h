#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <array>
#include <map>
#include <string>
#include <vector>

class Board;
class Creature;
class VM;
class Random;
class System;

class GameState {
public:
    struct MessageBox {
        std::string message;
        std::string portrait;
        int portraitSide;
        int runAfter;
    };

    static const int      PARTY_SIZE = 4;
    static const unsigned BOARD_WIDTH = 95;
    static const unsigned BOARD_HEIGHT = 95;

    GameState(VM &vm);
    ~GameState();

    Board* getBoard() {
        return mCurrentBoard;
    }
    Creature* getPlayer() {
        return mPlayer;
    }
    unsigned getTurn() const {
        return mTurnNumber;
    }
    int getDepth() const {
        return mDepth;
    }

    void pushMessage(const std::string &text) {
        mMessages.push_back(MessageBox{text});
    }
    void pushMessage(const MessageBox &m) {
        mMessages.insert(mMessages.begin(), m);
    }
    MessageBox popMessage() {
        if (mMessages.empty()) return MessageBox{""};
        MessageBox m = mMessages.back();
        mMessages.pop_back();
        return m;
    }
    bool hasMessages() const {
        return !mMessages.empty();
    }
    int messageCount() const {
        return mMessages.size();
    }

    void requestTick();
    bool hasTick() const;
    void tick();
    bool warpTo(int boardIndex, int x, int y);
    bool down();
    bool up();

    VM& getVM() {
        return vm;
    }

    System *system;
private:
    bool build(int forIndex);

    bool wantsTick;
    VM &vm;
    unsigned mTurnNumber;
    int mDepth;
    Board *mCurrentBoard;
    Creature *mPlayer;
    std::vector<MessageBox> mMessages;
    std::map<int, Board*> mBoards;
};

#endif