add_library(
        IR_lib STATIC
        Type.cpp
        User.cpp
        Value.cpp
        BasicBlock.cpp
        Constant.cpp
        Function.cpp
        GlobalVariable.cpp
        Instruction.cpp
        Module.cpp
        IRprinter.cpp
)

target_link_libraries(
        IR_lib
        LLVMSupport        
)
