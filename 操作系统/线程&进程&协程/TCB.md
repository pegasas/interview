# 线程控制块TCB
以下是线程控制块的声明。
struct tcb {
  u32_t状态;
  struct reg_context thread_context;
  void * stack;
  struct thread_info thread_params;
  u32_t executionTime;
  struct tcb * recoveryTask;
  u32_t sched_field;
  u32_t magic_key;
};
unsigned int status
该字段保存当前线程的状态信息。它可以是THREAD_ON_CPU，THREAD_READY，THREAD_SUSPENDED THREAD_BLOCKED，THREAD_EXITED或THREAD_MISSED_DEADLINE之一。

内核不直接更新此字段。内核线程库通过Scheduler API通知调度程序有关线程的状态。调度程序对象可以保持此字段的一致性。例如，当线程退出时，内核调用调度程序API的函数set_mode（curr_thread，THREAD_EXITED）。这种调用调度程序函数来更新状态的方法有助于，当线程状态发生变化时，sheduler需要做额外的工作（除了设置它的状态）。例如，对于周期性线程，它可能会在退出时重置它们。

struct reg_context thread_context
此结构存储线程的上下文。结构reg_context是特定于体系结构的。该字段仅由内核线程库访问。（调度程序对象不应该乱用它）。

struct thread_info thread_params
该字段包含初始线程参数，如启动函数，堆栈大小，截止日期等。此信息是重置线程所必需的。

void * stack
该字段是指向线程堆栈的指针。

u32_t executionTime
此字段可用于保留线程的分析信息。目前尚未使用此功能。

u32_t sched_field
该字段供调度程序对象使用。内核从未访问过该字段。

通常，调度程序对象将使用此字段来构造tcb的数据结构。例如，如果调度程序对象将就绪线程存储在列表中，则该字段将用作下一个指针。

u32_t magic_key
该字段用于调试，并应在最终版本中消失。
