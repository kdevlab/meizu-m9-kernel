/* linux/arch/arm/plat-s5p/devs.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Base S5P platform device definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <../../../drivers/staging/android/timed_gpio.h>
#include <../../../sound/soc/codecs/tlv320aic36.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/dma.h>
#include <mach/adc.h>
#include <mach/leds-gpio.h>

#include <plat/devs.h>
#include <plat/gpio-cfg.h>
#include <plat/irqs.h>
#include <plat/fb.h>
#include <plat/fimc.h>
#include <plat/csis.h>
#include <plat/fimg2d.h>
#include <plat/media.h>
#include <plat/jpeg.h>
#include <mach/media.h>
#include <s3cfb.h>
#include <linux/switch.h>

/* RTC */
static struct resource s5p_rtc_resource[] = {
	[0] = {
		.start = S3C_PA_RTC,
		.end   = S3C_PA_RTC + 0xff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_RTC_ALARM,
		.end   = IRQ_RTC_ALARM,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_RTC_TIC,
		.end   = IRQ_RTC_TIC,
		.flags = IORESOURCE_IRQ
	}
};

struct platform_device s5p_device_rtc = {
	.name             = "s3c2410-rtc",
	.id               = -1,
	.num_resources    = ARRAY_SIZE(s5p_rtc_resource),
	.resource         = s5p_rtc_resource,
};

#ifdef CONFIG_S5P_ADC
/* ADCTS */
static struct resource s3c_adc_resource[] = {
	[0] = {
		.start = SAMSUNG_PA_ADC,
		.end   = SAMSUNG_PA_ADC + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_PENDN,
		.end   = IRQ_PENDN,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_ADC,
		.end   = IRQ_ADC,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_adc = {
	.name		  = "s3c-adc",
	.id               = -1,
	.num_resources	  = ARRAY_SIZE(s3c_adc_resource),
	.resource	  = s3c_adc_resource,
};

void __init s3c_adc_set_platdata(struct s3c_adc_mach_info *pd)
{
	struct s3c_adc_mach_info *npd;

	npd = kmalloc(sizeof(*npd), GFP_KERNEL);
	if (npd) {
		memcpy(npd, pd, sizeof(*npd));
		s3c_device_adc.dev.platform_data = npd;
	} else {
		printk(KERN_ERR "no memory for ADC platform data\n");
	}
}
#endif /* CONFIG_S5P_ADC */

#if defined(CONFIG_VIDEO_MFC51) || defined(CONFIG_VIDEO_MFC50)
static struct resource s5p_mfc_resources[] = {
	[0] = {
		.start	= S5P_PA_MFC,
		.end	= S5P_PA_MFC + S5P_SZ_MFC - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_MFC,
		.end	= IRQ_MFC,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_mfc = {
	.name		= "mfc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_mfc_resources),
	.resource	= s5p_mfc_resources,
};
#endif

#if defined(CONFIG_S5P_DEV_FB)
static struct resource s3cfb_resource[] = {
	[0] = {
		.start = S5P_PA_LCD,
		.end   = S5P_PA_LCD + S5P_SZ_LCD - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_LCD1,
		.end   = IRQ_LCD1,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_LCD0,
		.end   = IRQ_LCD0,
		.flags = IORESOURCE_IRQ,
	},
};

static u64 fb_dma_mask = 0xffffffffUL;

struct platform_device s3c_device_fb = {
	.name		  = "s3cfb",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3cfb_resource),
	.resource	  = s3cfb_resource,
	.dev		  = {
		.dma_mask		= &fb_dma_mask,
		.coherent_dma_mask	= 0xffffffffUL
	}
};

static struct s3c_platform_fb default_fb_data __initdata = {
#if defined(CONFIG_CPU_S5PV210_EVT0)
	.hw_ver	= 0x60,
#else
	.hw_ver	= 0x62,
#endif
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_WORD | FB_SWAP_HWORD,
};

void __init s3cfb_set_platdata(struct s3c_platform_fb *pd)
{
	struct s3c_platform_fb *npd;
	struct s3cfb_lcd *lcd;
	phys_addr_t pmem_start;
	int i, default_win, num_overlay_win;
	int frame_size;

	if (!pd)
		pd = &default_fb_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fb), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		for (i = 0; i < npd->nr_wins; i++)
			npd->nr_buffers[i] = 1;

		default_win = npd->default_win;
		num_overlay_win = CONFIG_FB_S3C_NUM_OVLY_WIN;

		if (num_overlay_win >= default_win) {
			printk(KERN_WARNING "%s: NUM_OVLY_WIN should be less than default \
					window number. set to 0.\n", __func__);
			num_overlay_win = 0;
		}

		for (i = 0; i < num_overlay_win; i++)
			npd->nr_buffers[i] = CONFIG_FB_S3C_NUM_BUF_OVLY_WIN;
		npd->nr_buffers[default_win] = CONFIG_FB_S3C_NR_BUFFERS;

		lcd = (struct s3cfb_lcd *)npd->lcd;
		frame_size = (lcd->width * lcd->height * 4);

		s3cfb_get_clk_name(npd->clk_name);
	//	npd->backlight_onoff = NULL;
		npd->clk_on = s3cfb_clk_on;
		npd->clk_off = s3cfb_clk_off;

		/* set starting physical address & size of memory region for overlay
		 * window */
		pmem_start = s5p_get_media_memory_bank(S5P_MDEV_FIMD, 1);
		for (i = 0; i < num_overlay_win; i++) {
			npd->pmem_start[i] = pmem_start;
			npd->pmem_size[i] = frame_size * npd->nr_buffers[i];
			pmem_start += npd->pmem_size[i];
		}

		/* set starting physical address & size of memory region for default
		 * window */
		npd->pmem_start[default_win] = pmem_start;
		npd->pmem_size[default_win] = frame_size * npd->nr_buffers[default_win];

		s3c_device_fb.dev.platform_data = npd;
	}
}
#endif

#if defined(CONFIG_VIDEO_FIMC) || defined(CONFIG_CPU_FREQ) /* TODO: use existing dev */
static struct resource s3c_fimc0_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC0,
		.end	= S5P_PA_FIMC0 + S5P_SZ_FIMC0 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC0,
		.end	= IRQ_FIMC0,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc0 = {
	.name		= "s3c-fimc",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_fimc0_resource),
	.resource	= s3c_fimc0_resource,
};

static struct s3c_platform_fimc default_fimc0_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
	.hw_ver	= 0x45,
};

void __init s3c_fimc0_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc0_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc0_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;

		npd->hw_ver = 0x45;

		/* starting physical address of memory region */
		npd->pmem_start = s5p_get_media_memory_bank(S5P_MDEV_FIMC0, 1);
		/* size of memory region */
		npd->pmem_size = s5p_get_media_memsize_bank(S5P_MDEV_FIMC0, 1);

		s3c_device_fimc0.dev.platform_data = npd;
	}
}

static struct resource s3c_fimc1_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC1,
		.end	= S5P_PA_FIMC1 + S5P_SZ_FIMC1 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC1,
		.end	= IRQ_FIMC1,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc1 = {
	.name		= "s3c-fimc",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s3c_fimc1_resource),
	.resource	= s3c_fimc1_resource,
};

static struct s3c_platform_fimc default_fimc1_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
	.hw_ver	= 0x50,
};

void __init s3c_fimc1_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc1_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc1_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;

		npd->hw_ver = 0x50;

		/* starting physical address of memory region */
		npd->pmem_start = s5p_get_media_memory_bank(S5P_MDEV_FIMC1, 1);
		/* size of memory region */
		npd->pmem_size = s5p_get_media_memsize_bank(S5P_MDEV_FIMC1, 1);

		s3c_device_fimc1.dev.platform_data = npd;
	}
}

static struct resource s3c_fimc2_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC2,
		.end	= S5P_PA_FIMC2 + S5P_SZ_FIMC2 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC2,
		.end	= IRQ_FIMC2,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc2 = {
	.name		= "s3c-fimc",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(s3c_fimc2_resource),
	.resource	= s3c_fimc2_resource,
};

static struct s3c_platform_fimc default_fimc2_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
	.hw_ver	= 0x45,
};

void __init s3c_fimc2_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc2_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc2_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;

		npd->hw_ver = 0x45;

		/* starting physical address of memory region */
		npd->pmem_start = s5p_get_media_memory_bank(S5P_MDEV_FIMC2, 1);
		/* size of memory region */
		npd->pmem_size = s5p_get_media_memsize_bank(S5P_MDEV_FIMC2, 1);

		s3c_device_fimc2.dev.platform_data = npd;
	}
}

static struct resource s3c_ipc_resource[] = {
	[0] = {
		.start	= S5P_PA_IPC,
		.end	= S5P_PA_IPC + S5P_SZ_IPC - 1,
		.flags	= IORESOURCE_MEM,
	},
};

struct platform_device s3c_device_ipc = {
	.name		= "s3c-ipc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_ipc_resource),
	.resource	= s3c_ipc_resource,
};
#endif

/* JPEG controller  */
static struct s3c_platform_jpeg default_jpeg_data __initdata = {
	.max_main_width		= 2560,
	.max_main_height	= 1920,
	.max_thumb_width	= 0,
	.max_thumb_height	= 0,
};

void __init s3c_jpeg_set_platdata(struct s3c_platform_jpeg *pd)
{
	struct s3c_platform_jpeg *npd;

	if (!pd)
		pd = &default_jpeg_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_jpeg), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else
		s3c_device_jpeg.dev.platform_data = npd;
}

static struct resource s3c_jpeg_resource[] = {
	[0] = {
		.start = S5PV210_PA_JPEG,
		.end   = S5PV210_PA_JPEG + S5PV210_SZ_JPEG - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_JPEG,
		.end   = IRQ_JPEG,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_jpeg = {
	.name             = "s3c-jpg",
	.id               = -1,
	.num_resources    = ARRAY_SIZE(s3c_jpeg_resource),
	.resource         = s3c_jpeg_resource,
};

/* G3D */
struct platform_device s3c_device_g3d = {
	.name		= "pvrsrvkm",
	.id		= -1,
};

struct platform_device s3c_device_lcd = {
	.name		= "s3c_lcd",
	.id		= -1,
};

/* rotator interface */
static struct resource s5p_rotator_resource[] = {
	[0] = {
		.start = S5P_PA_ROTATOR,
		.end   = S5P_PA_ROTATOR + S5P_SZ_ROTATOR - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_ROTATOR,
		.end   = IRQ_ROTATOR,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_rotator = {
	.name		= "s5p-rotator",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_rotator_resource),
	.resource	= s5p_rotator_resource
};

/* TVOUT interface */
static struct resource s5p_tvout_resources[] = {
	[0] = {
		.start  = S5P_PA_TVENC,
		.end    = S5P_PA_TVENC + S5P_SZ_TVENC - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = S5P_PA_VP,
		.end    = S5P_PA_VP + S5P_SZ_VP - 1,
		.flags  = IORESOURCE_MEM,
	},
	[2] = {
		.start  = S5P_PA_MIXER,
		.end    = S5P_PA_MIXER + S5P_SZ_MIXER - 1,
		.flags  = IORESOURCE_MEM,
	},
	[3] = {
		.start  = S5P_PA_HDMI,
		.end    = S5P_PA_HDMI + S5P_SZ_HDMI - 1,
		.flags  = IORESOURCE_MEM,
	},
	[4] = {
		.start  = S5P_I2C_HDMI_PHY,
		.end    = S5P_I2C_HDMI_PHY + S5P_I2C_HDMI_SZ_PHY - 1,
		.flags  = IORESOURCE_MEM,
	},
	[5] = {
		.start  = IRQ_MIXER,
		.end    = IRQ_MIXER,
		.flags  = IORESOURCE_IRQ,
	},
	[6] = {
		.start  = IRQ_HDMI,
		.end    = IRQ_HDMI,
		.flags  = IORESOURCE_IRQ,
	},
	[7] = {
		.start  = IRQ_TVENC,
		.end    = IRQ_TVENC,
		.flags  = IORESOURCE_IRQ,
	},
};

struct platform_device s5p_device_tvout = {
	.name           = "s5p-tvout",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s5p_tvout_resources),
	.resource       = s5p_tvout_resources,
};

/* CEC */
static struct resource s5p_cec_resources[] = {
	[0] = {
		.start  = S5P_PA_CEC,
		.end    = S5P_PA_CEC + S5P_SZ_CEC - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_CEC,
		.end    = IRQ_CEC,
		.flags  = IORESOURCE_IRQ,
	},
};

struct platform_device s5p_device_cec = {
	.name           = "s5p-cec",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s5p_cec_resources),
	.resource       = s5p_cec_resources,
};

/* HPD */
struct platform_device s5p_device_hpd = {
	.name           = "s5p-hpd",
	.id             = -1,
};

#ifdef CONFIG_USB_SUPPORT
#ifdef CONFIG_USB_ARCH_HAS_EHCI
 /* USB Host Controlle EHCI registrations */
static struct resource s3c_usb__ehci_resource[] = {
	[0] = {
		.start = S5P_PA_USB_EHCI,
		.end   = S5P_PA_USB_EHCI  + S5P_SZ_USB_EHCI - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_UHOST,
		.end   = IRQ_UHOST,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_usb_ehci_dmamask = 0xffffffffUL;

struct platform_device s3c_device_usb_ehci = {
	.name		= "s5p-ehci",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usb__ehci_resource),
	.resource	= s3c_usb__ehci_resource,
	.dev		= {
		.dma_mask = &s3c_device_usb_ehci_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};
#endif /* CONFIG_USB_ARCH_HAS_EHCI */

#ifdef CONFIG_USB_ARCH_HAS_OHCI
/* USB Host Controlle OHCI registrations */
static struct resource s3c_usb__ohci_resource[] = {
	[0] = {
		.start = S5P_PA_USB_OHCI,
		.end   = S5P_PA_USB_OHCI  + S5P_SZ_USB_OHCI - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_UHOST,
		.end   = IRQ_UHOST,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_usb_ohci_dmamask = 0xffffffffUL;

struct platform_device s3c_device_usb_ohci = {
	.name		= "s5p-ohci",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usb__ohci_resource),
	.resource	= s3c_usb__ohci_resource,
	.dev		= {
		.dma_mask = &s3c_device_usb_ohci_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};
#endif /* CONFIG_USB_ARCH_HAS_EHCI */

/* USB Device (Gadget)*/
static struct resource s3c_usbgadget_resource[] = {
	[0] = {
		.start	= S3C_PA_OTG,
		.end	= S3C_PA_OTG + S3C_SZ_OTG - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_OTG,
		.end	= IRQ_OTG,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_usbgadget = {
	.name		= "s3c-usbgadget",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usbgadget_resource),
	.resource	= s3c_usbgadget_resource,
};
#endif

#ifdef CONFIG_MEIZU_M9W_KEYBOARD
#define BUTTON_FILTER_TIME 50
static struct gpio_keys_button m9w_keyboard_table[] = {
	{KEY_HOME,        		GPIO_MEIZU_KEY_HOME,			1,	"Home button",	EV_KEY, 	1,	BUTTON_FILTER_TIME, 0},
	{KEY_VOLUMEUP,   	GPIO_MEIZU_KEY_VOL_UP,   		1,	"Volume up",		EV_KEY, 	0,	BUTTON_FILTER_TIME, 0},
	{KEY_VOLUMEDOWN, 	GPIO_MEIZU_KEY_VOL_DOWN, 		1,	"Volume down",	EV_KEY, 	0,	BUTTON_FILTER_TIME, 0},
	{KEY_POWER,			GPIO_MEIZU_KEY_POWER,			1,	"power",			EV_KEY, 	1,	BUTTON_FILTER_TIME, 0},
};

static struct gpio_keys_platform_data m9w_key_data = {
	.buttons  = m9w_keyboard_table,
	.nbuttons = ARRAY_SIZE(m9w_keyboard_table),
};

struct platform_device m9w_keyboard = {
 	.name = "m9w_keyboard",
	.dev  = {
		.platform_data = &m9w_key_data,
	},
	.id   = -1,
};
#endif

#ifdef CONFIG_SWITCH_GPIO
static struct gpio_switch_platform_data m9w_headset_switch_data = {
   .name = "h2w",
   .gpio = GPIO_MEIZU_KEY_EAR,
};
struct platform_device m9w_switch_gpio = {
 	.name = "switch-gpio",
	.dev  = {
		.platform_data = &m9w_headset_switch_data,
	},
	.id   = -1,
};
#endif

static struct resource s5p_fimg2d_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMG2D,
		.end		= S5P_PA_FIMG2D + S5P_SZ_FIMG2D - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_2D,
		.end		= IRQ_2D,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_fimg2d = {
	.name		= "s5p-fimg2d",
	.id			= -1,
	.num_resources	= ARRAY_SIZE(s5p_fimg2d_resource),
	.resource	= s5p_fimg2d_resource
};
EXPORT_SYMBOL(s5p_device_fimg2d);

static struct fimg2d_platdata default_fimg2d_data __initdata = {
	.parent_clkname = "mout_g2d0",
	.clkname = "sclk_fimg2d",
	.gate_clkname = "fimg2d",
	.clkrate = 250 * 1000000,
};

void __init s5p_fimg2d_set_platdata(struct fimg2d_platdata *pd)
{
	struct fimg2d_platdata *npd;

	if (!pd)
		pd = &default_fimg2d_data;

	npd = kmemdup(pd, sizeof(*pd), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "no memory for fimg2d platform data\n");
	else
		s5p_device_fimg2d.dev.platform_data = npd;
}

#if defined(CONFIG_ANDROID_TIMED_GPIO)
static struct timed_gpio timed_gpios[] = {
	{
		.name = "vibrator",
		.gpio = LED_MOTO_GPIO,
		.max_timeout = 1500,		// 1.5s
		.active_low = 0,
	},
};

static struct timed_gpio_platform_data timed_gpio_data = {
	.num_gpios	= ARRAY_SIZE(timed_gpios),
	.gpios		= timed_gpios,
};

struct platform_device m9w_timed_gpios = {
	.name	= "timed-gpio",
	.id		= -1,
	.dev		= {
		.platform_data = &timed_gpio_data,
	},
};
#endif
#ifdef CONFIG_LEDS_M9W
static struct m9w_led_platdata led_pdata_key = {
#ifdef CONFIG_HAVE_PWM
	.pwm_id		= 2,
	.max_brightness = 255,
	.pwm_period_ns  = 5000000,	// freq 200HZ
#endif	
	.gpio		= LED_KEY_GPIO,
	.flags		= M9W_LEDF_ACTLOW,
	.name		= "led-key",
	.def_trigger	= "backlight",
};

struct platform_device m9w_led_key = {
	.name	= "m9w_led",
	.id		= LED_KEY,
	.dev		= {
		.platform_data = &led_pdata_key,
	},
};
#endif
struct aic36_setup_data m9w_codec_pdata_aic36_setup = {
	.i2c_bus = 0,
	.i2c_address = 0x1b,
};
struct platform_device m9w_codec_dev = {
	.name	= "tlv320aic36-codec",
	.id		= -1,
	.dev		= {
		.platform_data = &m9w_codec_pdata_aic36_setup,
	},
};
