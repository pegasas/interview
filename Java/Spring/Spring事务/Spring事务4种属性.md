# Spring事务4种属性

Spring在TransactionDefinition接口中定义这些属性,以供PlatfromTransactionManager使用, PlatfromTransactionManager是spring事务管理的核心接口。

```
public interface TransactionDefinition {
  int getPropagationBehavior();//返回事务的传播行为。
  int getIsolationLevel();//返回事务的隔离级别，事务管理器根据它来控制另外一个事务可以看到本事务内的哪些数据。
  int getTimeout();//返回事务必须在多少秒内完成。
  boolean isReadOnly();//事务是否只读，事务管理器能够根据这个返回值进行优化，确保事务是只读的。 
}
```
