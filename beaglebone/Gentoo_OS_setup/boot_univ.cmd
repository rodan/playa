setenv devnum 1
setenv bootpart 1:1
setenv devtype mmc
setenv fdt_buffer 0x60000
setenv fdtovaddr 0x88080000
mmc dev 1
mmc rescan
run loadimage
setenv args_mmc 'run finduuid;setenv bootargs console=${console} ${optargs} root=PARTUUID=${uuid} ro rootfstype=${mmcrootfstype} quiet bone_capemgr.uboot_capemgr_enabled=1 coherent_pool=1M net.ifnames=0'
run args_mmc
load ${devtype} ${bootpart} ${fdtaddr} ${bootdir}/dtbs/am335x-boneblack.dtb
load ${devtype} ${bootpart} ${fdtovaddr} /lib/firmware/univ-bbb-EVA-00A0.dtbo
fdt addr ${fdtaddr}
fdt resize ${fdt_buffer}
fdt apply ${fdtovaddr}

bootz ${loadaddr} - ${fdtaddr}

