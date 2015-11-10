; ModuleID = 'lib/kernel/target/get_local_id.c'
target datalayout = "e-m:e-i1:8:16-i8:8:16-i64:64-f80:128-n32:64"
target triple = "riscv64-unknown-linux-gnu"

@_local_base_id_x = external global i64, align 8
@_local_base_id_y = external global i64, align 8
@_local_base_id_z = external global i64, align 8

declare i64 @llvm.hwacha.veidx()

; Function Attrs: nounwind
define i64 @_Z12get_local_idj(i32 %dimindx) #0 {
entry:
  %retval = alloca i64, align 8
  %dimindx.addr = alloca i32, align 4
  store i32 %dimindx, i32* %dimindx.addr, align 4
  %0 = load i32, i32* %dimindx.addr, align 4
  switch i32 %0, label %sw.default [
    i32 0, label %sw.bb
    i32 1, label %sw.bb.1
    i32 2, label %sw.bb.2
  ]

sw.bb:                                            ; preds = %entry
  %1 = load i64, i64* @_local_base_id_x, align 8
  %call = call i64 @llvm.hwacha.veidx()
  %add = add i64 %1, %call
  store i64 %add, i64* %retval
  br label %return

sw.bb.1:                                          ; preds = %entry
  %2 = load i64, i64* @_local_base_id_y, align 8
  store i64 %2, i64* %retval
  br label %return

sw.bb.2:                                          ; preds = %entry
  %3 = load i64, i64* @_local_base_id_z, align 8
  store i64 %3, i64* %retval
  br label %return

sw.default:                                       ; preds = %entry
  store i64 0, i64* %retval
  br label %return

return:                                           ; preds = %sw.default, %sw.bb.2, %sw.bb.1, %sw.bb
  %4 = load i64, i64* %retval
  ret i64 %4
}


attributes #0 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-features"="+rv64" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-features"="+rv64" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.7.0  (git@github.com:riscv/riscv-llvm e40d0933929057622d08a7b68fc90773d912ecaa)"}
