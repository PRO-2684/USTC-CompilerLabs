input=$1
filename="stu_${input}_codegen"
compare="./ll_cases/${input}.ll"

lli ${compare}
expected=$?
echo Expected result: $expected

cd ~/compile/2023ustc-jianmu-compiler/build
rm ${input}.s
rm ${input}
./${filename} > ${input}.s
loongarch64-unknown-linux-gnu-gcc -static ${input}.s -o ${input}
qemu-loongarch64 ./${input}
actual=$?
echo Actual result: $actual

if [ $expected -eq $actual ]; then
    echo "Test passed! 🎉"
else
    echo "Test failed! 😢"
fi
