# CPP 简介

C++是一门面向对象的语言，从名字可以看出，C++从C中获得了许多灵感。但是随着C++的发展，它和C的差异也越来越大，一个典型的例子是C是弱类型的语言，而C++越来越支持强类型。因此我们不能直接认为C++兼容C，而应该先了解二者的区别。好在本次实验并不需要你们使用高级的C++特性，所以在此简单介绍一下部分特性便于理解。如果对C++有更深的兴趣，可以从Milo Yip的[游戏程序员的学习之路](https://github.com/miloyip/game-programmer/blob/master/game-programmer-zh-cn.jpg?raw=true)的C++部分开始看。

注：本介绍假设你有基本的C语言认知（略高于程设课标准），如果有不懂的C语言术语建议去搜索一下

## class

class是C++面向对象的基础，它相当于对C中的结构体的扩展。除了保留了原来的结构体成员（即成员对象），它增加了成员函数、访问控制、继承和多态等。

假设某类为`Animal`，一个它的实例为`animal`，我们可以在`Animal`的定义中增加函数声明`void eat();`，这样声明的函数即是成员函数。成员函数的作用域中自带一个`Animal*`类型的指针`this`，指向调用该成员函数的实例。我们可以通过`animal.eat()`一样，用类似访问成员的方法访问该函数。

```cpp
// 注：C++中struct也会定义结构体，只是访问控制的默认选项有所区别
struct Animal {
  void eat()
}
```

类的访问控制指的是在定义class时，可以用`public`与`private`标签，指定接下来的成员是私有或是公开成员。公开成员可以在外部函数使用该类的实例时访问，而内部成员只有该类的成员函数能访问。访问控制的作用是对使用者隐藏实现的细节，而关注于设计者想要公开的接口，从而让使用者能更容易理解如何使用该类。详细介绍在[access specifiers](https://en.cppreference.com/w/cpp/language/access)。

类的继承是一种面向对象语言常用的代码复用方法，也是一种非常直观的抽象方式。我们可以定义`struct Cat : Animal`来声明`Cat`类是`Animal`类的子类，也就是`Cat`继承了`Animal`类。此时，新的`Cat`类从`Animal`类中继承了`void eat();`成员函数，并且可以在此之上定义额外的成员函数`void nyan()`。同理，我们也可以定义`struct Dog : Animal`来定义`Dog`类。
```cpp
struct Cat : Animal {
  // 从Animal中继承了void eat();
  void nyan()
};

struct Dog : Animal {
  // 从Animal中继承了void eat();
  void wang()
};
```
我们可以通过合理的继承结构来将函数定义在合适的位置，使得大部分通用函数可以共享。

同学们可能会想到同是`Animal`，`Cat`和`Dog`可能会有相同名称与参数的函数，但是却有着不同的实现，这时我们就要用到虚函数了。子类中可以定义虚函数的实现，从而使得不同子类对于同一个名字的成员函数有不同实现。虚函数在调用时会通过虚函数表查找到对应的函数实现，而不是和普通类一样查找对应类型的函数实现。
```cpp
struct Animal {
  // = 0 表示该虚函数在Animal类中没有实现
  virtual void say() = 0;
};

struct Cat : Animal {
  // override表示覆盖父函数中的实现，下同
  void say() override {
    std::cout << "I'm a cat" << std::endl;
  }
};

struct Dog : Animal {
  void say() override{
    std::cout << "I'm a dog" << std::endl;
  }
};

// 试一试
int main() {
  Cat c;
  Dog d;
  Animal* a;
  c.say();
  d.say();
  a = &c;
  a->say();
  a = &d;
  a->say();
  return 0;
}
```

## 函数

C++中的函数可以重载，即可以有同名函数，但是要求它们的形参必须不同。如果想进一步了解，可以阅读[详细规则](https://en.cppreference.com/w/cpp/language/overload_resolution)。下面是函数重载的示例：

```cpp
struct Point {
  int x;
  int y;
};

struct Line {
  Point first;
  Point second;
};

void print(Point p) {
  printf("(%d, %d)", p.x, p.y);
}

void print(Line s) {
  print(s.first) // s.first == Point { ... }
  printf("->");
  print(s.second) // s.second == Point { ... }
}
```
我们可以看到上面的示例定义了两个`print`函数，并且它们的参数列表的类型不同。它们实际上是两个不同的函数（并且拥有不同的内部名字），但是C++能够正确的识别函数调用时使用了哪一个定义（前提是你正确使用了这一特性），并且在编译时就会链接上正确的实现。我们可以看到，这种特性非常符合人的直觉，并且没有带来额外开销。

## 泛型

不同于C中使用void指针来实现泛型函数（如`qsort`），C++中使用模板来帮助定义泛型类型与泛型函数等。由于模板过于复杂，这里不做深入介绍。这里你们需要理解的是，C++中的模板定义正如其名，在实例化前只是一个模板而不是参与编译的代码。只有在你使用的过程中指定了参数，编译器才会自动根据模板产生相应的代码，也就是实例化该参数对应的代码。比如`std::vector`是C++中常用的数组容器，在使用时必须指定参数，如果要实例化`int`类型的数组容器，必须要使用`std::vector<int>`。

## 内存分配

C中，只能使用标准库中的`malloc`与`free`来进行内存分配，并且需要手动在内存上初始化类型。C++中增加了`new`与`delete`关键字，你可以使用`new classname(params)`的完成申请一块内存，利用构造函数（`classname(params)`即代表调用`classname`类型的一个构造函数）来完成内存初始化。而`delete variable`可以调用变量对应类型函数的析构函数来完成数据结构的清理和回收内存。但是它存在着和C一样的二次回收导致报错或忘记回收导致内存泄漏的问题。于是C++11引入了许多智能指针类型，本实验中用到的有两种，分别是：

1. `std::shared_ptr`: 引用计数智能指针，使用一个共享变量来记录指针管理的对象被引用了几次。当对象引用计数为0时，说明当前该对象不再有引用，并且进程也无法再通过其它方式来引用它，也就意味着可以回收内存，这相当于低级的垃圾回收策略。
2. `std::unique_ptr`: 表示所有权的智能指针，该指针要求它所管理的对象智能有一次引用，主要用于语义上不允许共享的对象（比如`llvm::Module`）。当引用计数为0时，它也会回收内存。

## Debugging Seg Fault
Lab3/4 中，你可能会各种各样的段错误，不要害怕！clang 提供了一个工具来帮助我们解决内存泄漏。
```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Asan
make
```
然后再次运行你的错误代码， Asan 会提供更详细的报错信息。

注：要更换到别的 build type （如Debug或Release）时需要显式指定，否则 cmake 会使用 cached 的版本。
