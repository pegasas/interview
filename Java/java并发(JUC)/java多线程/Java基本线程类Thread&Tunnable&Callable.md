# Java基本线程类

Java多线程实现方式主要有3种：
继承Thread类
实现Runnable接口
实现Callable接口

(1)Thread类
(2)Runnable接口
(3)Callable接口

**Thread类相关方法：**
```
/* 当前线程可转让cpu控制权，让别的就绪状态线程运行（切换） */
public static Thread.yield()
/* 暂停一段时间 */
public static Thread.sleep()
/* 在一个线程中调用other.join(),将等待other执行完后才继续本线程。　　　　 */
public join()
/* 后两个函数皆可以被打断 */
public interrupte()
```
**关于中断**：它并不像stop方法那样会中断一个正在运行的线程。线程会不时地检测中断标识位，以判断线程是否应该被中断（中断标识值是否为true）。终端只会影响到wait状态、sleep状态和join状态。被打断的线程会抛出InterruptedException。
Thread.interrupted()检查当前线程是否发生中断，返回boolean
synchronized在获锁的过程中是不能被中断的。

中断是一个状态！interrupt()方法只是将这个状态置为true而已。所以说正常运行的程序不去检测状态，就不会终止，而wait等阻塞方法会去检查并抛出异常。如果在正常运行的程序中添加while(!Thread.interrupted()) ，则同样可以在中断后离开代码体

**Thread类最佳实践**：
写的时候最好要设置线程名称 Thread.name，并设置线程组 ThreadGroup，目的是方便管理。在出现问题的时候，打印线程栈 (jstack -pid) 一眼就可以看出是哪个线程出的问题，这个线程是干什么的。

**run()和start()**：
start:
无需等待run方法体代码执行完毕而直接继续执行下面的代码，两个线程并发地运行：
当前线程（从调用返回给 start 方法）和另一个线程（执行其 run 方法）。
这时此线程处于就绪（可运行）状态，并没有运行，一旦得到cpu时间片，就开始执行run()方法，
这里方法 run()称为线程体，它包含了要执行的这个线程的内容，Run方法运行结束，此线程随即终止。
多次启动一个线程是非法的。特别是当线程已经结束执行后，不能再重新启动。

run:
如果该线程是使用独立的 Runnable 运行对象构造的，则调用该 Runnable 对象的 run 方法；否则，该方法不执行任何操作并返回



**如何获取线程中的异常**
```
try{
	Thread t = new Thread(new Task());
	t.setUncaughtExceptionHandler(){
	@override
	public void uncaughtException(Thread t, Throwable e){
		System.out.println("catch 到了");
	}
	}
}catch(Exception e){
	System.out.println("catch 不到");
}
```
### Runnable
通过继承Thread类实现多线程有一定的局限性。因为在java中只支持单继承，一个类一旦继承了某个父类就无法再继承Thread类，比如学生类Student继承了person类，就无法再继承Thread类创建的线程。为了克服这种弊端，Thread类提供了另外一种构造方法Thread（Runnable target），其中Runnable是一个接口，它只有一个run（）方法。当通过Thread（Runnable target）构造方法创建一个线程对象时，只需该方法传递一个实现了Runnable接口的实例对象，这样创建的线程将调用实现了Runnable接口中的run（）方法作为运行代码，二不需要调用Thread类中的run（）方法。
```
package test;

public class example {
	public static void main( String[] args )
	{
		MyThread	myThread	= new MyThread();
		Thread		thread		= new Thread( myThread );
		thread.start();
		while ( true )
		{
			System.out.println( "Main方法在运行" );
		}
	}
}

class MyThread implements Runnable {
	public void run()
	{
		while ( true )
		{
			System.out.println( "MyThread类的run()方法在运行" );
		}
	}
}

```
### Callable

future模式：并发模式的一种，可以有两种形式，即无阻塞和阻塞，分别是isDone和get。其中Future对象用来存放该线程的返回值以及状态
```
ExecutorService e = Executors.newFixedThreadPool( 3 );
/* submit方法有多重参数版本，及支持callable也能够支持runnable接口类型. */
Future future = e.submit( new myCallable() );
future.isDone() /* return true,false 无阻塞 */
future.get()    /* return 返回值，阻塞直到该线程运行结束 */
```
