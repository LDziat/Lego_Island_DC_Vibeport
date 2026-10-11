export PATH=$PATH:/opt/toolchains/dc/sh-elf/bin:/opt/toolchains/dc/kos/utils/scramble
cd "/c/Users/ldzia/Desktop/AI Code Work/Lego Island/Dreamcast/isle"
sh-elf-objcopy -O binary build_kos/isle.elf build_kos/isle.bin
scramble build_kos/isle.bin build_kos/cd_root/1ST_READ.BIN
mkisofs -C 0,11702 -V LEGO_ISLAND -G ip.bin -r -J -l -m "*.ini" -o build_kos/isle.iso build_kos/cd_root
cdi4dc build_kos/isle.iso build_kos/isle.cdi
