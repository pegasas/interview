# nullptr

nullptr 出现的目的是为了替代 NULL。

在某种意义上来说，传统 C++ 会把 NULL、0 视为同一种东西，这取决于编译器如何定义 NULL，有些编译器会将 NULL 定义为 ((void*)0)，有些则会直接将其定义为 0。

C++ 不允许直接将 void * 隐式转换到其他类型，但如果 NULL 被定义为 ((void*)0)，那么当编译char *ch = NULL;时，NULL 只好被定义为 0。

而这依然会产生问题，将导致了 C++ 中重载特性会发生混乱，考虑：

void foo(char *);
void foo(int);

对于这两个函数来说，如果 NULL 又被定义为了 0 那么 foo(NULL); 这个语句将会去调用 foo(int)，从而导致代码违反直观。

为了解决这个问题，C++11 引入了 nullptr 关键字，专门用来区分空指针、0。

nullptr 的类型为 nullptr_t，能够隐式的转换为任何指针或成员指针的类型，也能和他们进行相等或者不等的比较。

当需要使用 NULL 时候，养成直接使用 nullptr的习惯。
