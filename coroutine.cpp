#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include"coroutine.h"

//初始化协程数组
Schedule::Schedule() : coroutines(std::vector<Coroutine*>(COROUTINE_SIZE, nullptr))
{
    cur_id = -1;
    max_id = 0;
}

//销毁每个协程
Schedule::~Schedule()
{
    for(int i = 0; i < max_id; i++)
    {
        coroutine_destory(i);
    }
}

//判断调度器的协程是否全部结束
bool Schedule::finish()
{
    if(cur_id != -1)
    {
        return false;
    }
    //有协程未结束
    for(int i = 0; i < max_id; i++)
    {
        if(coroutines[i]->state != DEAD)
        {
            return false;
        }
    }
    return true;
}

//获取当前协程id
int Schedule::curId() const
{
    return cur_id;
}

//辅助函数，上下文入口
void* run(uint32_t low, uint32_t high)
{
    uintptr_t ptr = (uintptr_t)low | ((uintptr_t)high << 32);
    Schedule* s = (Schedule*)ptr;
    //执行回调函数，然后关闭协程
    if(s->cur_id != -1)
    {
        Coroutine* co = s->coroutines[s->cur_id];
        co->call_back(s, co->args);
        co->state = DEAD;
        s->cur_id = -1;
    }
    return nullptr;
}

//协程创建
int Schedule::coroutine_create(std::function<void(Schedule*, void*)> call_back, void* args)
{
    Coroutine* co = nullptr;
    int i = 0;
    //找到第一个协程为空的位置
    for(; i < max_id; i++)
    {
        if(coroutines[i]->state == DEAD)
            break;
    }
    //都不为空则新建一个
    if(i == max_id || coroutines[i] == nullptr)
    {
        coroutines[i] = new Coroutine();
        ++max_id;
    }

    //初始化协程
    co = coroutines[i];
    co->call_back = call_back;
    co->args = args;
    co->state = READY;
    
    //保存当前上下文
    getcontext(&co->ctx);

    //设置保存上下文的栈
    co->ctx.uc_stack.ss_sp = &co->stack[0];
    co->ctx.uc_stack.ss_size = STACK_SIZE;
    co->ctx.uc_flags = 0;
    //设置下一个切换点
    co->ctx.uc_link = &main;

    //设置入口函数
    uintptr_t ptr = (uintptr_t)this;
    makecontext(&co->ctx, (void(*)())&run, 2, (uint32_t)ptr, (uint32_t)(ptr >> 32));

    return i;
}

//获取协程状态
State Schedule::coroutine_state(int id)
{
    assert(id >= 0 && id < max_id);

    Coroutine* co = coroutines[id];
    if(co == nullptr)
    {
        return DEAD;
    }
    return co->state;
}

//协程退出
void Schedule::coroutine_yield()
{
    //找到当前协程，设置成挂起，然后回到调度器
    if(cur_id != -1)
    {
        Coroutine* co = coroutines[cur_id];
        cur_id = -1;
        co->state = SUSPEND;
        swapcontext(&co->ctx, &main);
    }
}

//协程切换
void Schedule::coroutine_resume(int id)
{
    assert(id >= 0 && id < max_id);

    Coroutine* co = coroutines[id];
    if(co && co->state == SUSPEND)
    {
        co->state = RUNNING;
        cur_id = id;
        swapcontext(&main, &co->ctx);
    }
}

//销毁协程
void Schedule::coroutine_destory(int id)
{
    Coroutine* co = coroutines[id];
    if(co != nullptr)
    {
        delete co;
        co = nullptr;
    }
}

//协程运行
void Schedule::coroutine_running(int id)
{
    assert(id >= 0 && id < max_id);

    if(coroutine_state(id) == DEAD)
    {
        return;
    }

    Coroutine* co = coroutines[id];
    co->state = RUNNING;
    cur_id = id;

    swapcontext(&main, &co->ctx);
}