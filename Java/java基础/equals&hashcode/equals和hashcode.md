# equals和hashcode

java对于equals方法和hashCode方法是这样规定的：

(1)如果两个对象相同（equals方法返回true），那么它们的hashCode值一定要相同；

(2)如果两个对象的hashCode相同，它们并不一定相同。当然，你未必要按照要求去做，但是如果你违背了上述原则就会发现在使用容器时，相同的对象可以出现在Set集合中，同时增加新元素的效率会大大下降（对于使用哈希存储的系统，如果哈希码频繁的冲突将会造成存取性能急剧下降）。

首先equals方法必须满足自反性（x.equals(x)必须返回true）、对称性（x.equals(y)返回true时，y.equals(x)也必须返回true）、传递性（x.equals(y)和y.equals(z)都返回true时，x.equals(z)也必须返回true）和一致性（当x和y引用的对象信息没有被修改时，多次调用x.equals(y)应该得到同样的返回值），而且对于任何非null值的引用x，x.equals(null)必须返回false。

实现高质量的equals方法的诀窍包括：

使用==操作符检查”参数是否为这个对象的引用”；
使用instanceof操作符检查”参数是否为正确的类型”；
对于类中的关键属性，检查参数传入对象的属性是否与之相匹配；
编写完equals方法后，问自己它是否满足对称性、传递性、一致性；
重写equals时总是要重写hashCode；
不要将equals方法参数中的Object对象替换为其他的类型，在重写时不要忘掉@Override注解。
