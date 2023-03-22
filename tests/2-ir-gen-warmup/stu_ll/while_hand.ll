define dso_local i32 @main() #0 {
    %1 = alloca i32 ;a
    %2 = alloca i32 ;i
    store i32 10, i32* %1 ;a = 10
    store i32 0, i32* %2 ;i = 1
    ;i < 10
    %3 = load i32, i32* %2
    %4 = icmp slt i32 %3, 10
    br i1 %4, label %5, label %12
5:
    ; i = i + 1
    %6 = load i32, i32* %2
    %7 = add i32 %6, 1
    store i32 %7, i32* %2
    ; a = a + i
    %8 = load i32, i32* %1
    %9 = load i32, i32* %2
    %10 = add i32 %8, %9
    store i32 %10, i32* %1
    ; i < 10
    %11 = icmp slt i32 %9, 10
    br i1 %11, label %5, label %12
12:
    %13 = load i32, i32* %1 
    ret i32 %13
}