define dso_local i32 @main() #0 {
    %1 = alloca float
    store float 0x40163851E0000000, float* %1 ;a = 5.555 5.555的二进制表示小数部分是无限的,因此需近似
    %2 = load float, float* %1 
    %3 = fcmp ogt float %2, 1.0 ; a>1
    br i1 %3, label %4, label %5
4:
    ret i32 233
5:
    ret i32 0
}