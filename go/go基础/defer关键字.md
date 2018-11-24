# defer关键字

# 用法

defer语句会在该函数结束的时候被调用，即使后面的语句运行时出现异常了defer语句仍然会被执行。
执行顺序先进后出。

```
package main

import (
    "fmt"
)

func main() {
    defer_call()
}

func defer_call() {
    defer func() { fmt.Println("打印前") }()
    defer func() { fmt.Println("打印中") }()
    defer func() { fmt.Println("打印后") }()

    panic("触发异常")
}
```

输出：
```
打印后
打印中
打印前
panic: 触发异常
```

# 原理

在golang当中，defer代码块会在函数调用链表中增加一个函数调用。这个函数调用不是普通的函数调用，而是会在函数正常返回，也就是return之后添加一个函数调用。
