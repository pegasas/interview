# c++异常堆栈跟踪

本文将简单介绍Linux中C/C++程序运行时堆栈获取，首先来看backtrace系列函数——使用范围适合于没有安装GDB或者想要快速理清楚函数调用顺序的情况 ，头文件execinfo.h

int backtrace (void \*\*buffer, int size);

该函数用来获取当前线程的调用堆栈，获取的信息将会被存放在buffer中，它是一个指针数组。参数size用来指定buffer中可以保存多少个void* 元素。函数返回值是实际获取的指针个数，最大不超过size大小在buffer中的指针实际是从堆栈中获取的返回地址,每一个堆栈框架有一个返回地址。注意某些编译器的优化选项对获取正确的调用堆栈有干扰，另外内联函数没有堆栈框架；删除框架指针也会使无法正确解析堆栈内容。

char \*\*backtrace_symbols (void \*const \*buffer, int size);

该函数将从backtrace函数获取的信息转化为一个字符串数组。参数buffer是从backtrace函数获取的数组指针，size是该数组中的元素个数(backtrace的返回值)，函数返回值是一个指向字符串数组的指针,它的大小同buffer相同。每个字符串包含了一个相对于buffer中对应元素的可打印信息。它包括函数名，函数的偏移地址和实际的返回地址。backtrace_symbols生成的字符串都是malloc出来的，但是不要最后一个一个的free，因为backtrace_symbols会根据backtrace给出的callstack层数，一次性的将malloc出来一块内存释放，所以，只需要在最后free返回指针就OK了。

void backtrace_symbols_fd (void \*const \*buffer, int size, int fd);  

该函数与backtrace_symbols函数具有相同的功能，不同的是它不会给调用者返回字符串数组，而是将结果写入文件描述符为fd的文件中，每个函数对应一行。它不需要调用malloc函数，因此适用于有可能调用该函数会失败的情况。
