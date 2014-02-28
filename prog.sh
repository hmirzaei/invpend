arm-none-eabi-objdump -D gcc/invpend.axf > invpend.lst
sed -i '31s/.*/reg msp 0x'$(cat invpend.lst | grep 20000000: | cut -c 11-18)'/' fury_ft2232.cfg
sed -i '32s/.*/resume 0x'$(cat invpend.lst | grep "<ResetISR>:" | cut -c 1-8)'/' fury_ft2232.cfg
openocd --file fury_ft2232.cfg
