define dso_local i32 @main() #0 {
    %1 = alloca [10 x i32] ; a[10]
    %2 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 0 ;a[0]的指针
    %3 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 1 ;a[1]的指针
    store i32 10, i32* %2 ;a[0] = 10;
    %4 = load i32, i32* %2 
    %5 = mul i32 %4, 2
    store i32 %5, i32* %3 ;a[1] = a[0] * 2;
    %6 = load i32, i32* %3
    ret i32 %6
}