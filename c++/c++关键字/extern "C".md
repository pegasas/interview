# extern "C"

被 extern 限定的函数或变量是 extern 类型的
被 extern "C" 修饰的变量和函数是按照 C 语言方式编译和连接的
extern "C" 的作用是让 C++ 编译器将 extern "C" 声明的代码当作 C 语言代码处理，可以避免 C++ 因符号修饰导致代码不能和C语言库中的符号进行链接的问题。

extern "C"用法

```
#ifdef __cplusplus
extern "C" {
#endif

void *memset(void *, int, size_t);

#ifdef __cplusplus
}
#endif
```
