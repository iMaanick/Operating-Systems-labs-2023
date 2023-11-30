#ifndef OS_LABS_MESSAGE_H
#define OS_LABS_MESSAGE_H


enum class State {  
    ALIVE,
    DEAD
};

struct Message {
    State state;
    int num;

    Message(State st = State::ALIVE, int num = 0) : state(st), num(num) {
    }
};

#endif //OS_LABS_MESSAGE_H
