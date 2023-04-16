#pragma once
#include<vector>
#include<ucontext.h>
#include<functional>

#define STACK_SIZE 128 * 1024
#define COROUTINE_SIZE 128

enum State
{
    DEAD,
    RUNNING,
    READY,
    SUSPEND
};

class Schedule;

struct Coroutine
{
    std::function<void(Schedule*, void*)> call_back;
    void* args;
    ucontext_t ctx;
    std::vector<char> stack;
    State state;

    Coroutine() : stack(std::vector<char>(STACK_SIZE)), state(READY) {}
};

class Schedule
{
public:
    Schedule();
    ~Schedule();

    Schedule(const Schedule&) = delete;
    Schedule& operator=(const Schedule&) = delete;

    bool finish();
    int curId() const;

    int  coroutine_create(std::function<void(Schedule*, void*)> call_back, void* args);
    void coroutine_yield();
    void coroutine_resume(int id);
    void coroutine_running(int id);

private:
    State coroutine_state(int id);
    void coroutine_destory(int id);
    friend void* run(uint32_t low, uint32_t high);

private:
    std::vector<Coroutine*> coroutines;
    int cur_id;
    int max_id;
    ucontext_t main;
};