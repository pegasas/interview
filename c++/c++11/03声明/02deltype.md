# deltype

delctype 类型说明符。从表达式的类型推断要定义的变量的类型。
delctype ()括号内可以是变量、表达式或函数返回值。

```
decltype(f()) x="abc  ";//并不真正调用f()
decltype(x) z=y;
```
