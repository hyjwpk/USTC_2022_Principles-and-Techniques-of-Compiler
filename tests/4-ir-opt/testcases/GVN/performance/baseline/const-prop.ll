declare i32 @input()

declare void @output(i32)

declare void @outputFloat(float)

declare void @neg_idx_except()

define void @main() {
label_entry:
  %op0 = call i32 @input()
  br label %label1
label1:                                                ; preds = %label_entry, %label11
  %op2 = phi i32 [ 0, %label_entry ], [ 711082625, %label11 ]
  %op7 = phi i32 [ 0, %label_entry ], [ %op37, %label11 ]
  %op8 = icmp slt i32 %op7, %op0
  %op9 = zext i1 %op8 to i32
  %op10 = icmp ne i32 %op9, 0
  br i1 %op10, label %label11, label %label38
label11:                                                ; preds = %label1
  %op37 = add i32 %op7, 1
  br label %label1
label38:                                                ; preds = %label1
  call void @output(i32 %op2)
  ret void
}
