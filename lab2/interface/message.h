#ifndef OS_LABS_MESSAGE_H
#define OS_LABS_MESSAGE_H




struct Message {
    int state;
    int num;

    Message(int st = 0, int num = 0) : state(st), num(num) {
    }
};

#endif //OS_LABS_MESSAGE_H
