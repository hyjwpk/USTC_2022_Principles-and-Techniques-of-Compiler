; ModuleID = 'cminus'
source_filename = "./test.cminus"

@matrix = global [20000000 x i32] zeroinitializer
@ad = global [100000 x i32] zeroinitializer
@len = global i32 zeroinitializer
declare i32 @input()

declare void @output(i32)

declare void @outputFloat(float)

declare void @neg_idx_except()

define void @readarray() {
label_entry:
  br label %label0
label0:                                                ; preds = %label_entry, %label11
  %op1 = phi i32 [ 0, %label_entry ], [ %op13, %label11 ]
  %op2 = load i32, i32* @len
  %op3 = icmp slt i32 %op1, %op2
  %op4 = zext i1 %op3 to i32
  %op5 = icmp ne i32 %op4, 0
  br i1 %op5, label %label6, label %label9
label6:                                                ; preds = %label0
  %op7 = call i32 @input()
  %op8 = icmp slt i32 %op1, 0
  br i1 %op8, label %label10, label %label11
label9:                                                ; preds = %label0
  ret void
label10:                                                ; preds = %label6
  call void @neg_idx_except()
  ret void
label11:                                                ; preds = %label6
  %op12 = getelementptr [100000 x i32], [100000 x i32]* @ad, i32 0, i32 %op1
  store i32 %op7, i32* %op12
  %op13 = add i32 %op1, 1
  br label %label0
}
define i32 @transpose(i32 %arg0, i32* %arg1, i32 %arg2) {
label_entry:
  %op4 = sdiv i32 %arg0, %arg2
  br label %label5
label5:                                                ; preds = %label_entry, %label25
  %op8 = phi i32 [ 0, %label_entry ], [ %op26, %label25 ]
  %op9 = icmp slt i32 %op8, %op4
  %op10 = zext i1 %op9 to i32
  %op11 = icmp ne i32 %op10, 0
  br i1 %op11, label %label12, label %label13
label12:                                                ; preds = %label5
  br label %label15
label13:                                                ; preds = %label5
  ret i32 -1
label15:                                                ; preds = %label12, %label29
  %op17 = phi i32 [ 0, %label12 ], [ %op31, %label29 ]
  %op18 = icmp slt i32 %op17, %arg2
  %op19 = zext i1 %op18 to i32
  %op20 = icmp ne i32 %op19, 0
  br i1 %op20, label %label21, label %label25
label21:                                                ; preds = %label15
  %op22 = icmp slt i32 %op8, %op17
  %op23 = zext i1 %op22 to i32
  %op24 = icmp ne i32 %op23, 0
  br i1 %op24, label %label27, label %label32
label25:                                                ; preds = %label15
  %op26 = add i32 %op8, 1
  br label %label5
label27:                                                ; preds = %label21
  %op28 = add i32 %op17, 1
  br label %label29
label29:                                                ; preds = %label27, %label57
  %op31 = phi i32 [ %op28, %label27 ], [ %op59, %label57 ]
  br label %label15
label32:                                                ; preds = %label21
  %op33 = mul i32 %op8, %arg2
  %op34 = add i32 %op33, %op17
  %op35 = icmp slt i32 %op34, 0
  br i1 %op35, label %label36, label %label37
label36:                                                ; preds = %label32
  call void @neg_idx_except()
  ret i32 0
label37:                                                ; preds = %label32
  %op38 = getelementptr i32, i32* %arg1, i32 %op34
  %op39 = load i32, i32* %op38
  br i1 %op35, label %label43, label %label44
label43:                                                ; preds = %label37
  call void @neg_idx_except()
  ret i32 0
label44:                                                ; preds = %label37
  %op46 = load i32, i32* %op38
  %op47 = mul i32 %op17, %op4
  %op48 = add i32 %op47, %op8
  %op49 = icmp slt i32 %op48, 0
  br i1 %op49, label %label50, label %label51
label50:                                                ; preds = %label44
  call void @neg_idx_except()
  ret i32 0
label51:                                                ; preds = %label44
  %op52 = getelementptr i32, i32* %arg1, i32 %op48
  store i32 %op46, i32* %op52
  br i1 %op35, label %label56, label %label57
label56:                                                ; preds = %label51
  call void @neg_idx_except()
  ret i32 0
label57:                                                ; preds = %label51
  store i32 %op39, i32* %op38
  %op59 = add i32 %op17, 1
  br label %label29
}
define i32 @main() {
label_entry:
  %op0 = call i32 @input()
  %op1 = call i32 @input()
  store i32 %op1, i32* @len
  call void @readarray()
  br label %label2
label2:                                                ; preds = %label_entry, %label11
  %op3 = phi i32 [ 0, %label_entry ], [ %op13, %label11 ]
  %op4 = icmp slt i32 %op3, %op0
  %op5 = zext i1 %op4 to i32
  %op6 = icmp ne i32 %op5, 0
  br i1 %op6, label %label7, label %label9
label7:                                                ; preds = %label2
  %op8 = icmp slt i32 %op3, 0
  br i1 %op8, label %label10, label %label11
label9:                                                ; preds = %label2
  br label %label14
label10:                                                ; preds = %label7
  call void @neg_idx_except()
  ret i32 0
label11:                                                ; preds = %label7
  %op12 = getelementptr [20000000 x i32], [20000000 x i32]* @matrix, i32 0, i32 %op3
  store i32 %op3, i32* %op12
  %op13 = add i32 %op3, 1
  br label %label2
label14:                                                ; preds = %label9, %label25
  %op15 = phi i32 [ 0, %label9 ], [ %op29, %label25 ]
  %op16 = load i32, i32* @len
  %op17 = icmp slt i32 %op15, %op16
  %op18 = zext i1 %op17 to i32
  %op19 = icmp ne i32 %op18, 0
  br i1 %op19, label %label20, label %label23
label20:                                                ; preds = %label14
  %op21 = getelementptr [20000000 x i32], [20000000 x i32]* @matrix, i32 0, i32 0
  %op22 = icmp slt i32 %op15, 0
  br i1 %op22, label %label24, label %label25
label23:                                                ; preds = %label14
  br label %label30
label24:                                                ; preds = %label20
  call void @neg_idx_except()
  ret i32 0
label25:                                                ; preds = %label20
  %op26 = getelementptr [100000 x i32], [100000 x i32]* @ad, i32 0, i32 %op15
  %op27 = load i32, i32* %op26
  %op28 = call i32 @transpose(i32 %op0, i32* %op21, i32 %op27)
  %op29 = add i32 %op15, 1
  br label %label14
label30:                                                ; preds = %label23, %label45
  %op31 = phi i32 [ 0, %label23 ], [ %op49, %label45 ]
  %op32 = phi i32 [ 0, %label23 ], [ %op50, %label45 ]
  %op33 = load i32, i32* @len
  %op34 = icmp slt i32 %op32, %op33
  %op35 = zext i1 %op34 to i32
  %op36 = icmp ne i32 %op35, 0
  br i1 %op36, label %label37, label %label40
label37:                                                ; preds = %label30
  %op38 = mul i32 %op32, %op32
  %op39 = icmp slt i32 %op32, 0
  br i1 %op39, label %label44, label %label45
label40:                                                ; preds = %label30
  %op41 = icmp slt i32 %op31, 0
  %op42 = zext i1 %op41 to i32
  %op43 = icmp ne i32 %op42, 0
  br i1 %op43, label %label51, label %label53
label44:                                                ; preds = %label37
  call void @neg_idx_except()
  ret i32 0
label45:                                                ; preds = %label37
  %op46 = getelementptr [20000000 x i32], [20000000 x i32]* @matrix, i32 0, i32 %op32
  %op47 = load i32, i32* %op46
  %op48 = mul i32 %op38, %op47
  %op49 = add i32 %op31, %op48
  %op50 = add i32 %op32, 1
  br label %label30
label51:                                                ; preds = %label40
  %op52 = sub i32 0, %op31
  br label %label53
label53:                                                ; preds = %label40, %label51
  %op54 = phi i32 [ %op31, %label40 ], [ %op52, %label51 ]
  call void @output(i32 %op54)
  ret i32 0
}
