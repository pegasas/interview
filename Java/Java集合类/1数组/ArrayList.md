# ArrayList

一、ArrayList简介

ArrayList是可以动态增长和缩减的索引序列，它是基于数组实现的List类。

该类封装了一个动态再分配的Object[]数组，每一个类对象都有一个capacity属性，表示它们所封装的Object[]数组的长度，当向ArrayList中添加元素时，该属性值会自动增加。如果想ArrayList中添加大量元素，可使用ensureCapacity方法一次性增加capacity，可以减少增加重分配的次数提高性能。

ArrayList的用法和Vector向类似，但是Vector是一个较老的集合，具有很多缺点，不建议使用。另外，ArrayList和Vector的区别是：ArrayList是线程不安全的，当多条线程访问同一个ArrayList集合时，程序需要手动保证该集合的同步性，而Vector则是线程安全的。

ArrayList与Collection关系如下图：

![avatar][ArrayList与Collection关系]

二、ArrayList源码分析

```
public class ArrayList<E> extends AbstractList<E> implements List<E>, RandomAccess, Cloneable, java.io.Serializable
{
    private static final long serialVersionUID = 8683452581122892189L;
    //默认的初始容量为10
    private static final int DEFAULT_CAPACITY = 10;
    private static final Object[] EMPTY_ELEMENTDATA = {};
    private static final Object[] DEFAULTCAPACITY_EMPTY_ELEMENTDATA = {};
    transient Object[] elementData;
    // ArrayList中实际数据的数量
    private int size;
    public ArrayList(int initialCapacity) //带初始容量大小的构造函数
    {
        if (initialCapacity > 0)   //初始容量大于0,实例化数组
        {
            this.elementData = new Object[initialCapacity];
        }
        else if (initialCapacity == 0) //初始化等于0，将空数组赋给elementData
        {
            this.elementData = EMPTY_ELEMENTDATA;
        }
        else    //初始容量小于，抛异常
        {
            throw new IllegalArgumentException("Illegal Capacity: "+ initialCapacity);
        }
    }
    public ArrayList()  //无参构造函数,默认容量为10
    {
        this.elementData = DEFAULTCAPACITY_EMPTY_ELEMENTDATA;
    }
    public ArrayList(Collection<? extends E> c)  //创建一个包含collection的ArrayList
    {
        elementData = c.toArray(); //返回包含c所有元素的数组
        if ((size = elementData.length) != 0)
        {
            if (elementData.getClass() != Object[].class)
                elementData = Arrays.copyOf(elementData, size, Object[].class);//复制指定数组，使elementData具有指定长度
        }
        else
        {
            //c中没有元素
            this.elementData = EMPTY_ELEMENTDATA;
        }
    }
    //将当前容量值设为当前实际元素大小
    public void trimToSize()
    {
        modCount++;
        if (size < elementData.length)
        {
            elementData = (size == 0)? EMPTY_ELEMENTDATA:Arrays.copyOf(elementData, size);
        }
    }

    //将集合的capacit增加minCapacity
    public void ensureCapacity(int minCapacity)
    {
        int minExpand = (elementData != DEFAULTCAPACITY_EMPTY_ELEMENTDATA)?0:DEFAULT_CAPACITY;
        if (minCapacity > minExpand)
        {
            ensureExplicitCapacity(minCapacity);
        }
    }
    private void ensureCapacityInternal(int minCapacity)
    {
        if (elementData == DEFAULTCAPACITY_EMPTY_ELEMENTDATA)
        {
            minCapacity = Math.max(DEFAULT_CAPACITY, minCapacity);
        }
        ensureExplicitCapacity(minCapacity);
    }
    private void ensureExplicitCapacity(int minCapacity)
    {
        modCount++;
        if (minCapacity - elementData.length > 0)
            grow(minCapacity);
    }
    private static final int MAX_ARRAY_SIZE = Integer.MAX_VALUE - 8;
    private void grow(int minCapacity)
    {
        int oldCapacity = elementData.length;
　　　　　//注意此处扩充capacity的方式是将其向右一位再加上原来的数，实际上是扩充了1.5倍
        int newCapacity = oldCapacity + (oldCapacity >> 1);
        if (newCapacity - minCapacity < 0)
            newCapacity = minCapacity;
        if (newCapacity - MAX_ARRAY_SIZE > 0)
            newCapacity = hugeCapacity(minCapacity);
        elementData = Arrays.copyOf(elementData, newCapacity);
    }
    private static int hugeCapacity(int minCapacity)
    {
        if (minCapacity < 0) // overflow
            throw new OutOfMemoryError();
        return (minCapacity > MAX_ARRAY_SIZE) ?
            Integer.MAX_VALUE :
            MAX_ARRAY_SIZE;
    }
    //返回ArrayList的大小
    public int size()
    {
        return size;
    }
    //判断ArrayList是否为空
    public boolean isEmpty() {
        return size == 0;
    }
    //判断ArrayList中是否包含Object(o)
    public boolean contains(Object o) {
        return indexOf(o) >= 0;
    }
    //正向查找，返回ArrayList中元素Object(o)的索引位置
    public int indexOf(Object o)
    {
        if (o == null) {
            for (int i = 0; i < size; i++)
                if (elementData[i]==null)
                    return i;
        }
        else
        {
            for (int i = 0; i < size; i++)
                if (o.equals(elementData[i]))
                    return i;
        }
        return -1;
    }
    //逆向查找，返回返回ArrayList中元素Object(o)的索引位置
    public int lastIndexOf(Object o) {
        if (o == null) {
            for (int i = size-1; i >= 0; i--)
                if (elementData[i]==null)
                    return i;
        } else {
            for (int i = size-1; i >= 0; i--)
                if (o.equals(elementData[i]))
                    return i;
        }
        return -1;
    }
    //返回此 ArrayList实例的浅拷贝。
    public Object clone()
    {
        try
        {
            ArrayList<?> v = (ArrayList<?>) super.clone();
            v.elementData = Arrays.copyOf(elementData, size);
            v.modCount = 0;
            return v;
        }
        catch (CloneNotSupportedException e) {
            // this shouldn't happen, since we are Cloneable
            throw new InternalError(e);
        }
    }
    //返回一个包含ArrayList中所有元素的数组
    public Object[] toArray() {
        return Arrays.copyOf(elementData, size);
    }
    @SuppressWarnings("unchecked")
    public <T> T[] toArray(T[] a) {
        if (a.length < size)
            return (T[]) Arrays.copyOf(elementData, size, a.getClass());
        System.arraycopy(elementData, 0, a, 0, size);
        if (a.length > size)
            a[size] = null;
        return a;
    }
    @SuppressWarnings("unchecked")
    E elementData(int index) {
        return (E) elementData[index];
    }

    //返回至指定索引的值
    public E get(int index)
    {
        rangeCheck(index); //检查给定的索引值是否越界
        return elementData(index);
    }

    //将指定索引上的值替换为新值，并返回旧值
    public E set(int index, E element)
    {
        rangeCheck(index);
        E oldValue = elementData(index);
        elementData[index] = element;
        return oldValue;
    }

    //将指定的元素添加到此列表的尾部
    public boolean add(E e)
    {
        ensureCapacityInternal(size + 1);
        elementData[size++] = e;
        return true;
    }

    // 将element添加到ArrayList的指定位置
    public void add(int index, E element) {
        rangeCheckForAdd(index);
        ensureCapacityInternal(size + 1);

        //从指定源数组中复制一个数组，复制从指定的位置开始，到目标数组的指定位置结束。
        //arraycopy(被复制的数组, 从第几个元素开始复制, 要复制到的数组, 从第几个元素开始粘贴, 一共需要复制的元素个数)
        //即在数组elementData从index位置开始，复制到index+1位置,共复制size-index个元素
        System.arraycopy(elementData, index, elementData, index + 1,size - index);
        elementData[index] = element;
        size++;
    }

    //删除ArrayList指定位置的元素
    public E remove(int index)
    {
        rangeCheck(index);
        modCount++;
        E oldValue = elementData(index);
        int numMoved = size - index - 1;
        if (numMoved > 0)
            System.arraycopy(elementData, index+1, elementData, index,numMoved);
        elementData[--size] = null; //将原数组最后一个位置置为null
        return oldValue;
    }

    //移除ArrayList中首次出现的指定元素(如果存在)。
    public boolean remove(Object o) {
        if (o == null)
        {
            for (int index = 0; index < size; index++)
                if (elementData[index] == null)
                {
                    fastRemove(index);
                    return true;
                }
        }
        else
        {
            for (int index = 0; index < size; index++)
                if (o.equals(elementData[index]))
                {
                    fastRemove(index);
                    return true;
                }
        }
        return false;
    }

    //快速删除指定位置的元素
    private void fastRemove(int index)
    {
        modCount++;
        int numMoved = size - index - 1;
        if (numMoved > 0)
            System.arraycopy(elementData, index+1, elementData, index, numMoved);
        elementData[--size] = null;
    }

    //清空ArrayList，将全部的元素设为null
    public void clear()
    {
        modCount++;
        for (int i = 0; i < size; i++)
            elementData[i] = null;
        size = 0;
    }

    //按照c的迭代器所返回的元素顺序，将c中的所有元素添加到此列表的尾部
    public boolean addAll(Collection<? extends E> c) {
        Object[] a = c.toArray();
        int numNew = a.length;
        ensureCapacityInternal(size + numNew);  // Increments modCount
        System.arraycopy(a, 0, elementData, size, numNew);
        size += numNew;
        return numNew != 0;
    }

    //从指定位置index开始，将指定c中的所有元素插入到此列表中
    public boolean addAll(int index, Collection<? extends E> c) {
        rangeCheckForAdd(index);
        Object[] a = c.toArray();
        int numNew = a.length;
        ensureCapacityInternal(size + numNew);  // Increments modCount
        int numMoved = size - index;
        if (numMoved > 0)
            //先将ArrayList中从index开始的numMoved个元素移动到起始位置为index+numNew的后面去
            System.arraycopy(elementData, index, elementData, index + numNew, numMoved);
        //再将c中的numNew个元素复制到起始位置为index的存储空间中去
        System.arraycopy(a, 0, elementData, index, numNew);
        size += numNew;
        return numNew != 0;
    }

    //删除fromIndex到toIndex之间的全部元素
    protected void removeRange(int fromIndex, int toIndex)
    {
        modCount++;
        //numMoved为删除索引后面的元素个数
        int numMoved = size - toIndex;
        //将删除索引后面的元素复制到以fromIndex为起始位置的存储空间中去
        System.arraycopy(elementData, toIndex, elementData, fromIndex,numMoved);
        int newSize = size - (toIndex-fromIndex);
        //将ArrayList后面(toIndex-fromIndex)个元素置为null
        for (int i = newSize; i < size; i++)
        {
            elementData[i] = null;
        }
        size = newSize;
    }

    //检查索引是否越界
    private void rangeCheck(int index)
    {
        if (index >= size)
            throw new IndexOutOfBoundsException(outOfBoundsMsg(index));
    }
    private void rangeCheckForAdd(int index)
    {
        if (index > size || index < 0)
            throw new IndexOutOfBoundsException(outOfBoundsMsg(index));
    }

    private String outOfBoundsMsg(int index) {
        return "Index: "+index+", Size: "+size;
    }

    //删除ArrayList中包含在c中的元素
    public boolean removeAll(Collection<?> c)
    {
        Objects.requireNonNull(c);
        return batchRemove(c, false);
    }

    //删除ArrayList中除包含在c中的元素，和removeAll相反
    public boolean retainAll(Collection<?> c)
    {
        Objects.requireNonNull(c);  //检查指定对象是否为空
        return batchRemove(c, true);
    }

    private boolean batchRemove(Collection<?> c, boolean complement) {
        final Object[] elementData = this.elementData;
        int r = 0, w = 0;
        boolean modified = false;
        try
        {
            for (; r < size; r++)
                if (c.contains(elementData[r]) == complement)  //判断c中是否有elementData[r]元素

                    elementData[w++] = elementData[r];
        }
        finally
        {
            if (r != size)
            {
                System.arraycopy(elementData, r, elementData, w, size - r);
                w += size - r;
            }
            if (w != size)
            {
                // clear to let GC do its work
                for (int i = w; i < size; i++)
                    elementData[i] = null;
                modCount += size - w;
                size = w;
                modified = true;
            }
        }
        return modified;
    }

    //将ArrayList的“容量，所有的元素值”都写入到输出流中
    private void writeObject(java.io.ObjectOutputStream s) throws java.io.IOException
    {
        int expectedModCount = modCount;
        s.defaultWriteObject();
        //写入数组大小
        s.writeInt(size);
        //写入所有数组的元素
        for (int i=0; i<size; i++) {
            s.writeObject(elementData[i]);
        }
        if (modCount != expectedModCount) {
            throw new ConcurrentModificationException();
        }
    }

    //先将ArrayList的“大小”读出，然后将“所有的元素值”读出
    private void readObject(java.io.ObjectInputStream s)
        throws java.io.IOException, ClassNotFoundException {
        elementData = EMPTY_ELEMENTDATA;
        s.defaultReadObject();
        s.readInt(); // ignored
        if (size > 0) {
            // be like clone(), allocate array based upon size not capacity
            ensureCapacityInternal(size);
            Object[] a = elementData;
            // Read in all elements in the proper order.
            for (int i=0; i<size; i++) {
                a[i] = s.readObject();
            }
        }
    }
```

三、ArrayList遍历方式

ArrayList支持3种遍历方式
1、通过迭代器遍历：

```
Iterator iter = list.iterator();
while (iter.hasNext())
{
    System.out.println(iter.next());
}
```

2、随机访问，通过索引值去遍历，由于ArrayList实现了RandomAccess接口

```
int size = list.size();
for (int i=0; i<size; i++)
{
    System.out.println(list.get(i));
}
```

3、for循环遍历：

```
for(String str:list) {
  System.out.println(str);
}
```

[ArrayList与Collection关系]:data:image/png;base64,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
