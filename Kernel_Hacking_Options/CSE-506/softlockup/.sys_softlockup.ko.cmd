cmd_/usr/src/hw3-cse506g23/CSE-506/softlockup/sys_softlockup.ko := ld -r -m elf_x86_64  -z max-page-size=0x200000 -T ./scripts/module-common.lds  --build-id  -o /usr/src/hw3-cse506g23/CSE-506/softlockup/sys_softlockup.ko /usr/src/hw3-cse506g23/CSE-506/softlockup/sys_softlockup.o /usr/src/hw3-cse506g23/CSE-506/softlockup/sys_softlockup.mod.o ;  true
