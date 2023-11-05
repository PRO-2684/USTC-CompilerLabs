input=$1
filename="stu_${input}_codegen"
cd ~/compile/2023ustc-jianmu-compiler/build
rm ${input}.s
rm ${input}
./${filename} > ${input}.s
loongarch64-unknown-linux-gnu-gcc -static ${input}.s -o ${input}
qemu-loongarch64 ./${input}
echo $?
