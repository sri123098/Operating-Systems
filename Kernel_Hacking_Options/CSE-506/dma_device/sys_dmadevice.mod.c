#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xc5a3c142, "module_layout" },
	{ 0xd6a8ae06, "sysptr_dmadevice" },
	{ 0x37a0cba, "kfree" },
	{ 0xdf566a59, "__x86_indirect_thunk_r9" },
	{ 0x7e526bfa, "__x86_indirect_thunk_r10" },
	{ 0x97651e6c, "vmemmap_base" },
	{ 0x7cd8d75e, "page_offset_base" },
	{ 0x4c9d28b0, "phys_base" },
	{ 0xc5d19ea, "dma_ops" },
	{ 0x60efb6c9, "kmem_cache_alloc_trace" },
	{ 0x3c356f44, "kmalloc_caches" },
	{ 0x19caf60a, "device_register" },
	{ 0xf2212ce6, "dev_set_name" },
	{ 0xc5850110, "printk" },
	{ 0xba01c2dd, "device_unregister" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "FEB2F8DFF0A69F2B8C55E24");
