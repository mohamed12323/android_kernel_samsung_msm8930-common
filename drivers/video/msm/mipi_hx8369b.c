/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#define DEBUG

#include <mach/socinfo.h>
#include <linux/lcd.h>
#include "msm_fb.h"
#include <linux/gpio.h>
#include "mipi_dsi.h"
#include "mipi_hx8369b.h"
#include <mach/gpiomux.h>
#include <linux/gpio.h>

#if defined(CONFIG_FB_MDP4_ENHANCE)
#include "mdp4_video_enhance.h"
#elif defined(CONFIG_MDNIE_LITE_TUNING)
#include "mdnie_lite_tuning.h"
#endif

//static struct msm_panel_common_pdata *mipi_hx8369b_pdata;
static struct dsi_buf hx8369b_tx_buf;
static struct dsi_buf hx8369b_rx_buf;

static int mipi_hx8369b_bl_ctrl;

#if defined(CONFIG_BACKLIGHT_IC_KTD3102)
spinlock_t bl_ctrl_lock;
static int lcd_brightness = -1;
#endif

struct mutex dsi_tx_mutex;
/* common setting */

#define HX8369B_CMD_SETTLE 0
static char exit_sleep[2] = {0x11, 0x00};
static char display_on[2] = {0x29, 0x00};
static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};

static char hx8369b_boe1[] = {
	0xB9,
	0xFF, 0x83, 0x69
};
static char hx8369b_boe2[] = {
	0xBA,
	0x31, 0x00, 0x16, 0xCA, 0xB1, 0x0A, 0x00, 0x10, 0x28, 0x02, 0x21, 0x21, 0x9A, 0x1A, 0x8F
};

static char hx8369b_sdc1[] = {
        0xBA,
        0x11, 0x00, 0x16, 0xC6, 0x80, 0x0A, 0x00, 0x10, 0x24, 0x02, 0x21, 0x21, 0x9A, 0x11, 0x14,
};

static char hx8369b_boe3[] = {
		0xD5, /*GIPI*/
	/*1*/
		0x00, 0x00, 0x08, 0x03, 0x30, 
		0x00, 0x00, 0x10, 0x01, 0x00, 
		0x00, 0x00, 0x01, 0x39, 0x45,
		0x00, 0x00, 0x0C, 0x44, 0x39, 
	/*21*/
		0x47, 0x05, 0x00, 0x02, 0x04, 
		0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x03, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x37, 0x59, 
		0x18, 0x10, 0x00, 0x00, 0x05, 
		0x00, 0x00, 0x40, 0x28, 0x69, 
	/*51*/
		0x40, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x26, 0x49, 0x08, 0x00, 
		0x00, 0x00, 0x51, 0x00, 0x00, 
		0x51, 0x39, 0x78, 0x50, 0x00, 
	/*71*/
		0x00, 0x01, 0x00, 0x00, 0x00, 
		0x01, 0x00, 0x00, 0x00, 0x07, 
		0xF8, 0x0F, 0xFF, 0xFF, 0x07, 
		0xF8, 0x0F, 0xFF, 0xFF, 0x00, 
		0x20, 0x5A, 0xff, 0xff, 0xff,

};

static char hx8369b_sdc2[] = {
        0xD5,
/*1*/
        0x00, 0x00, 0x09, 0x03, 0x2D,
        0x00, 0x00, 0x12, 0x31, 0x23,
        0x00, 0x00, 0x10, 0x70, 0x37,
        0x00, 0x00, 0x0D, 0x01, 0x40,
/*21*/
        0x37, 0x00, 0x00, 0x00, 0x03,
        0x00, 0x00, 0x00, 0x00, 0x03,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x16, 0xEF, 0x00, 0x13, 0x57,
/*41*/
        0x71, 0x00, 0x00, 0x00, 0xEF,
        0xEF, 0x00, 0x64, 0x20, 0x06,
        0x00, 0x00, 0x00, 0xEF, 0xEF,
        0x00, 0x02, 0x46, 0x60, 0x00,
/*61*/
        0x00, 0x00, 0xEF, 0xEF, 0x00,
        0x75, 0x31, 0x17, 0x00, 0x00,
        0x00, 0xEF, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x03,
/*81*/
        0xFC, 0x0C, 0xFC, 0xFF, 0x0F,
        0xFC, 0x0C, 0xFC, 0xFF, 0x00,
        0x00, 0x5A,
};

static char hx8369b_boe4[] = {
	0x3A,
	0x70
};
static char hx8369b_boe5[] = {
	0xB1,
	0x12, 0x83, 0x77, 0x00, 0x10, 0x10, 0x1E, 0x1E, 0x0C, 0x1A,0x20,0xD6
};

static char hx8369b_sdc3[] = {
        0xB1,
        0x0A, 0x83, 0x77, 0x00, 0x8F, 0x0F, 0x1C, 0x1C, 0x0C, 0x2A, 0x20, 0xCE,
};

static char hx8369b_boe6[] = {
	0xB3,
	0x83, 0x00, 0x3A, 0x17
};

static char hx8369b_sdc4[] = {
        0xB3,
        0x03, 0x00, 0x30, 0x0B,
};

static char hx8369b_boe7[] = {
	0xB4,
	0x01
};

static char hx8369b_sdc5[] = {
        0xB4,
        0x02,
};

static char hx8369b_boe8[] = {
	0xB6,
	0x96, 0x96
};

static char hx8369b_sdc6[] = {
        0xB6,
        0xB1, 0xAE, 0x00,
};

static char hx8369b_sdc7[] = {
        0xBF,
        0x5F, 0x00, 0x00, 0x06,
};

static char hx8369b_boe9[] = {
	0xCC,
	0x0E
};

static char hx8369b_sdc8[] = {
        0xCC,
        0x0E,
};

static char hx8369b_sdc9[] = {
        0xC1,
        0x00,
};

static char hx8369b_boe10[] = {
	0xC0,
	0x73, 0x50, 0x00, 0x20, 0xC4, 0x00
};

static char hx8369b_sdc10[] = {
        0xC0,
        0x73, 0x50, 0x00, 0x2C, 0xC4, 0x00,
};

static char hx8369b_boe11[] = {
	0xE3,
	0x0F, 0x0F, 0x11, 0x11
};

static char hx8369b_sdc11[] = {
        0xE3,
        0x03, 0x03, 0x03, 0x03,
};

static char hx8369b_boe12[] = {
	0xEA,
	0x7A
};

static char hx8369b_sdc12[] = {
        0xEA,
        0x7A,
};

static char hx8369b_boe13[] = {
	0xC6,
	0x41, 0xFF, 0x7C
};

static char hx8369b_sdc13[] = {
        0xC6,
        0x41, 0xFF, 0x7D,
};

static char hx8369b_boe15[] = {
	0xE0, /*Gamma*/
	0x00, 0x1B, 0x21, 0x27, 0x30, 
	0x3F, 0x2D, 0x46, 0x0A, 0x11, 
	0x11, 0x14, 0x15, 0x15, 0x16, 
	0x13, 0x17, 0x00, 0x1B, 0x21, 
	0x27, 0x30, 0x3F, 0x2C, 0x45, 
	0x09, 0x0F, 0x11, 0x15, 0x17, 
	0x15, 0x16, 0x13, 0x17, 0x01,

};


static char hx8369b_sdc14[] = {
        0xE0,
        0x18, 0x21, 0x27, 0x35, 0x3A,
        0x3F, 0x32, 0x4C, 0x09, 0x0E,
        0x0E, 0x12, 0x13, 0x11, 0x13,
        0x18, 0x1D, 0x18, 0x21, 0x27,
        0x35, 0x3A, 0x3F, 0x32, 0x4C,
        0x09, 0x0E, 0x0E, 0x12, 0x13,
        0x11, 0x13, 0x18, 0x1D, 0x01,
};

static char hx8369b_boe16[] = {
	0xC1,
	0x00
};
static char cmd14[] = {
	0xC9,
	0x0F, 0x02
};

static char cmd15[] = {
	0x51,
	0xFF
};
static char cmd16[] = {
	0x53,
	0x24
};
static char cmd17[] = {
	0x55,
	0x01
};

static struct dsi_cmd_desc hx8369b_video_display_init_boe_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe1), hx8369b_boe1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe2), hx8369b_boe2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe3), hx8369b_boe3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe4), hx8369b_boe4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe5), hx8369b_boe5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe6), hx8369b_boe6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe7), hx8369b_boe7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe8), hx8369b_boe8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe9), hx8369b_boe9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe10), hx8369b_boe10},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe11), hx8369b_boe11},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe12), hx8369b_boe12},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe13), hx8369b_boe13},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe14), hx8369b_boe14},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe15), hx8369b_boe15},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe16), hx8369b_boe16},
	{DTYPE_DCS_WRITE, 1, 0, 0, 300, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(display_on), display_on},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(cmd14), cmd14},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(cmd15), cmd15},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(cmd16), cmd16},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(cmd17), cmd17},

};



static struct dsi_cmd_desc hx8369b_video_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc hx8369b_video_display_init_sdc_cmds[] = {
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369b_boe1), hx8369b_boe1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_sdc1), hx8369b_sdc1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_sdc2), hx8369b_sdc2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369b_sdc3), hx8369b_sdc3},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 180, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_sdc4), hx8369b_sdc4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_sdc5), hx8369b_sdc5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_sdc6), hx8369b_sdc6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_sdc7), hx8369b_sdc7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_sdc8), hx8369b_sdc8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_sdc9), hx8369b_sdc9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_sdc10), hx8369b_sdc10},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_sdc11), hx8369b_sdc11},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_sdc12), hx8369b_sdc12},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_sdc13), hx8369b_sdc13},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_sdc14), hx8369b_sdc14},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(cmd14), cmd14},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 300, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 150, sizeof(display_on), display_on},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(cmd14), cmd14},
        {DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(cmd15), cmd15},
        {DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(cmd16), cmd16},
        {DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(cmd17), cmd17},
};

static char cabcon1[] = {
        0x51,
        0xFF,
        0x3C,
};

static char cabcon2[] = {
        0x53,
        0x24,
        0x08,
};

static char cabcui[] = {
        0x55,
        0x01,
        0x1D,
};
#if 0
static char cabcstill[] = {
        0x55,
        0x02,
        0x1E,
};

static char cabcmoving[] = {
        0x55,
        0x03,
        0x2F,
};
#endif
static char cabcoff[] = {
        0x55,
        0x00,
        0x2C,
};

static struct dsi_cmd_desc hx8369b_video_display_cabcon_cmds[] = {
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369b_boe1), hx8369b_boe1},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd14), cmd14},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cabcon1), cabcon1},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cabcon2), cabcon2},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 5, sizeof(cabcui), cabcui},
};

static struct dsi_cmd_desc hx8369b_video_display_cabcoff_cmds[] = {
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369b_boe1), hx8369b_boe1},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd14), cmd14},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 5, sizeof(cabcoff), cabcoff},
};

#if defined(CONFIG_FB_MDP4_ENHANCE)
extern int is_negativeMode_on(void);
#endif


int cabc_switch;
int flag_lcd_on_off = 1;
void lcd_cabc_on(void)
{
	mutex_lock(&dsi_tx_mutex);
        printk("%s is called\n", __func__);
        mipi_dsi_cmds_tx(&hx8369b_tx_buf,
                        hx8369b_video_display_cabcon_cmds,
                        ARRAY_SIZE(hx8369b_video_display_cabcon_cmds));
	mutex_unlock(&dsi_tx_mutex);
}

void lcd_cabc_off(void)
{
	mutex_lock(&dsi_tx_mutex);
        printk("%s is called\n", __func__);
        mipi_dsi_cmds_tx(&hx8369b_tx_buf,
                        hx8369b_video_display_cabcoff_cmds,
                        ARRAY_SIZE(hx8369b_video_display_cabcoff_cmds));
	mutex_unlock(&dsi_tx_mutex);
}

static int mipi_hx8369b_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	int ret;
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;

	if (!mfd->cont_splash_done) {
		mfd->cont_splash_done = 1;
		return 0;
	}

#if defined(CONFIG_FB_MDP4_ENHANCE)
	is_negativeMode_on();
#endif	
	ret = gpio_get_value(65);
	printk("KERN_ERR lwj [LCD] mipi_hx8369b_lcd_on, gpio65=%d\n", ret);
	if (mipi->mode == DSI_VIDEO_MODE)
	{
		mutex_lock(&dsi_tx_mutex);
		if (ret)
			ret = mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_video_display_init_boe_cmds, ARRAY_SIZE(hx8369b_video_display_init_boe_cmds));
		else
			ret = mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_video_display_init_sdc_cmds, ARRAY_SIZE(hx8369b_video_display_init_sdc_cmds));
		mutex_unlock(&dsi_tx_mutex);
	}	
	if (cabc_switch)
		lcd_cabc_on();
	else
		lcd_cabc_off();
#if defined(CONFIG_BACKLIGHT_IC_KTD3102)
	pr_info("%s: DISP_BL_CONT_GPIO High\n", __func__);
	gpio_set_value(DISP_BL_CONT_GPIO, 1);
#endif
	flag_lcd_on_off = 1;
	return 0;
}

static int mipi_hx8369b_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;

	printk("KERN_ERR [LCD] mipi_hx8369b_lcd_off\n");
	if (mipi->mode == DSI_VIDEO_MODE)
	{
		mutex_lock(&dsi_tx_mutex);
		mipi_dsi_cmds_tx(&hx8369b_tx_buf,
			hx8369b_video_display_off_cmds,
			ARRAY_SIZE(hx8369b_video_display_off_cmds));
		mutex_unlock(&dsi_tx_mutex);
	}
#if defined(CONFIG_BACKLIGHT_IC_KTD3102)	
	pr_info("%s: DISP_BL_CONT_GPIO low-block\n", __func__);
	gpio_set_value(DISP_BL_CONT_GPIO, 0);
	lcd_brightness = -1;
#endif
	flag_lcd_on_off = 0;
	return 0;
}

static ssize_t mipi_hx8369b_wta_bl_ctrl(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	int err;

	err =  kstrtoint(buf, 0, &mipi_hx8369b_bl_ctrl);
	if (err)
		return ret;

	pr_info("%s: bl ctrl set to %d\n", __func__, mipi_hx8369b_bl_ctrl);

	return ret;
}

static DEVICE_ATTR(bl_ctrl, S_IWUSR, NULL, mipi_hx8369b_wta_bl_ctrl);

static struct attribute *mipi_hx8369b_fs_attrs[] = {
	&dev_attr_bl_ctrl.attr,
	NULL,
};

static struct attribute_group mipi_hx8369b_fs_attr_group = {
	.attrs = mipi_hx8369b_fs_attrs,
};

static int mipi_hx8369b_create_sysfs(struct platform_device *pdev)
{
	int rc;
	struct msm_fb_data_type *mfd = platform_get_drvdata(pdev);

	if (!mfd) {
		pr_err("%s: mfd not found\n", __func__);
		return -ENODEV;
	}
	if (!mfd->fbi) {
		pr_err("%s: mfd->fbi not found\n", __func__);
		return -ENODEV;
	}
	if (!mfd->fbi->dev) {
		pr_err("%s: mfd->fbi->dev not found\n", __func__);
		return -ENODEV;
	}
	rc = sysfs_create_group(&mfd->fbi->dev->kobj,
		&mipi_hx8369b_fs_attr_group);
	if (rc) {
		pr_err("%s: sysfs group creation failed, rc=%d\n",
			__func__, rc);
		return rc;
	}

	return 0;
}
#if defined(CONFIG_LCD_CLASS_DEVICE)
static struct lcd_ops mipi_lcd_props;
#endif
static int __devinit mipi_hx8369b_lcd_probe(struct platform_device *pdev)
{
	struct platform_device *pthisdev = NULL;
#if defined(CONFIG_LCD_CLASS_DEVICE)
        struct lcd_device *lcd_device;
        int ret;
#endif
#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
        struct backlight_device *bd;
#endif
	pr_debug("%s\n", __func__);

	if (pdev->id == 0) {
#if 0
		mipi_hx8369b_pdata = pdev->dev.platform_data;
		if (mipi_hx8369b_pdata->bl_lock)
			spin_lock_init(&mipi_hx8369b_pdata->bl_spinlock);
#endif
		mipi_hx8369b_bl_ctrl = 1;

		return 0;
	}

	pthisdev = msm_fb_add_device(pdev);
	mipi_hx8369b_create_sysfs(pthisdev);
	mutex_init(&dsi_tx_mutex);

#if defined(CONFIG_MDNIE_LITE_TUNING) \
	|| defined(CONFIG_FB_MDP4_ENHANCE)
	/*	mdnie sysfs create */
	init_mdnie_class();
#endif
#if defined(CONFIG_LCD_CLASS_DEVICE)
        lcd_device = lcd_device_register("panel", &pdev->dev, NULL,
                                        &mipi_lcd_props);

        if (IS_ERR(lcd_device)) {
                int ret = PTR_ERR(lcd_device);
                printk(KERN_ERR "lcd : failed to register device\n");
                return ret;
        }
#endif
#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
        bd = backlight_device_register("panel", &lcd_device->dev,
                                                NULL, NULL, NULL);
        if (IS_ERR(bd)) {
                ret = PTR_ERR(bd);
                pr_info("backlight : failed to register device\n");
                return ret;
        }
#endif
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_hx8369b_lcd_probe,
	.driver = {
		.name   = "mipi_hx8369b",
	},
};

#if defined(CONFIG_BACKLIGHT_IC_KTD3102)
#if 0
static int lux_tbl[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 26, 27, 28,	29, 30,
	31, 
};
#endif
static int mipi_hx8369b_set_brightness_level(int bl_level)
{
	int backlightlevel;
	int cd;

	if (!cabc_switch)
		switch (bl_level) {
#if 1 
		case 0 ... 19: 
			backlightlevel = 31; /*0*/
			break;
#endif
		case 20 ... 39:
			backlightlevel = 31; /* 32 */
			break;
		case 40 ... 49:
			backlightlevel = 30; /* 29 */
			break;
		case 50 ... 59:
			backlightlevel = 29;  /* 28 */
			break;
		case 60 ... 69:
			backlightlevel = 28;  /* 26 */
			break;
		case 70 ... 79:
			backlightlevel = 27;  /* 24 */
			break;
		case 80 ... 89:
			backlightlevel = 26;  /* 20 */
			break;
		case 90 ... 109:
			backlightlevel = 24;  /* 20 */
			break;
		case 110 ... 119:
			backlightlevel = 22;  /* 17 */
			break;
		case 120 ... 139:
			backlightlevel = 20;  /* 16 */
			break;
		case 140 ... 159:
			backlightlevel = 18;  /* 15 */
			break;
		case 160 ... 169:
			backlightlevel = 17;  /* 15 */
			break;
		case 170 ... 179:
			backlightlevel = 16;  /* 13 */
			break;
		case 180 ... 199:
			backlightlevel = 15;  /* 11 */
			break;
		case 200 ... 209:
			backlightlevel = 13;  /* 9 */
			break;
		case 210 ... 255:
			backlightlevel = 11;  /* 7 */
			break;
		default:
			backlightlevel = 31; /*32*/
			break;
		}
	else
		switch (bl_level) {
#if 1
                case 0 ... 19:
                        backlightlevel = 31; /*0*/
                        break;
#endif
                case 20 ... 39:
                        backlightlevel = 31; /* 31 */
                        break;
                case 40 ... 49:
                        backlightlevel = 30; /* 31 */
                        break;
                case 50 ... 59:
                        backlightlevel = 29; /* 30 */
                        break;
                case 60 ... 69:
                        backlightlevel = 28;  /* 28 */
                        break;
                case 70 ... 79:
                        backlightlevel = 27;  /* 26 */
                        break;
                case 80 ... 89:
                        backlightlevel = 26;  /* 20 */
                        break;
                case 90 ... 109:
                        backlightlevel = 25;  /* 21 */
                        break;
                case 110 ... 119:
                        backlightlevel = 23;  /* 20 */
                        break;
                case 120 ... 139:
                        backlightlevel = 22;  /* 18 */
                        break;
                case 140 ... 159:
                        backlightlevel = 21;  /* 18 */
                        break;
                case 160 ... 169:
                        backlightlevel = 20;  /* 17 */
                        break;
                case 170 ... 179:
                        backlightlevel = 19;  /* 16 */
                        break;
                case 180 ... 199:
                        backlightlevel = 17;  /* 15 */
                        break;
                case 200 ... 209:	
                        backlightlevel = 15;  /* 15 */
                        break;
                case 210 ... 255:
                        backlightlevel = 14 ;  /* 13 */
                        break;
                default:
                        backlightlevel = 31; /*32*/
                        break;
		}

	//cd = lux_tbl[backlightlevel - 1];
	cd = backlightlevel;
	return cd;
}

void mipi_hx8369b_set_backlight(int level)
{

	
	int tune_level = 0;

	spin_lock(&bl_ctrl_lock);	
	tune_level = level;

	pr_debug("[KTD3102] tune_level : %d, lcd_brightness : %d \n",tune_level,lcd_brightness);	
	if(tune_level != lcd_brightness)
	{
		if (!tune_level) {
			gpio_set_value(DISP_BL_CONT_GPIO, 0);
			mdelay(3);
			lcd_brightness = tune_level;
		} else {
		       int pulse;
			if (unlikely(lcd_brightness < 0)) {
				int val = gpio_get_value(DISP_BL_CONT_GPIO);
				if (val) {
					lcd_brightness = 0;
				gpio_set_value(DISP_BL_CONT_GPIO, 0);
				mdelay(3);
					pr_info("LCD Baklight init in boot time on kernel\n");
				}
			}
			if (!lcd_brightness) {
				gpio_set_value(DISP_BL_CONT_GPIO, 1);
				udelay(3);
				lcd_brightness = MAX_BRIGHTNESS_IN_BLU;
			}

			pulse = (tune_level - lcd_brightness + MAX_BRIGHTNESS_IN_BLU)
							% MAX_BRIGHTNESS_IN_BLU;

			for (; pulse > 0; pulse--) {
				gpio_set_value(DISP_BL_CONT_GPIO, 0);
				udelay(3);
				gpio_set_value(DISP_BL_CONT_GPIO, 1);
				udelay(3);
			}

			lcd_brightness = tune_level;
		}
	}
	mdelay(1);
	spin_unlock(&bl_ctrl_lock);
}


static void mipi_hx8369b_disp_backlight(struct msm_fb_data_type *mfd)
{
	struct mipi_panel_info *mipi;
#ifndef CONFIG_BACKLIGHT_IC_KTD3102
	struct dcs_cmd_req cmdreq;
#endif	
	static int bl_level_old;

	pr_info("%s Back light level:%d\n", __func__, mfd->bl_level);

	mipi  = &mfd->panel_info.mipi;
	if (bl_level_old == mfd->bl_level)
		goto end;
	if (!mfd->panel_power_on)
		goto end;

#if defined(CONFIG_BACKLIGHT_IC_KTD3102)

	mipi_hx8369b_set_backlight(mipi_hx8369b_set_brightness_level(mfd->bl_level));
	bl_level_old = mfd->bl_level;

#else /*for himax ldi HX8394-A*/
	mutex_lock(&mfd->dma->ov_mutex);
	/* mdp4_dsi_cmd_busy_wait: will turn on dsi clock also */
	mipi_dsi_mdp_busy_wait();

	WRDISBV[1] = (unsigned char)
		(msd.mpd->set_brightness_level(mfd->bl_level));

	cmdreq.cmds = himax_video_backlight_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(himax_video_backlight_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

	bl_level_old = mfd->bl_level;
	mutex_unlock(&mfd->dma->ov_mutex);
#endif

end:
	return;
	
}
#else


//static int old_bl_level;

static void mipi_hx8369b_set_backlight(struct msm_fb_data_type *mfd)
{
#if 0
	int bl_level;
	unsigned long flags;
	bl_level = mfd->bl_level;

	if (mipi_hx8369b_pdata->bl_lock) {
		if (!mipi_hx8369b_bl_ctrl) {
			/* Level received is of range 1 to bl_max,
			   We need to convert the levels to 1
			   to 31 */
			bl_level = (2 * bl_level * 31 + mfd->panel_info.bl_max)
					/(2 * mfd->panel_info.bl_max);
			if (bl_level == old_bl_level)
				return;

			if (bl_level == 0)
				mipi_hx8369b_pdata->backlight(0, 1);

			if (old_bl_level == 0)
				mipi_hx8369b_pdata->backlight(50, 1);

			spin_lock_irqsave(&mipi_hx8369b_pdata->bl_spinlock,
						flags);
			mipi_hx8369b_pdata->backlight(bl_level, 0);
			spin_unlock_irqrestore(&mipi_hx8369b_pdata->bl_spinlock,
						flags);
			old_bl_level = bl_level;
		} else {
			mipi_hx8369b_pdata->backlight(bl_level, 1);
		}
	} else {
		mipi_hx8369b_pdata->backlight(bl_level, mipi_hx8369b_bl_ctrl);
	}
#endif
}
#endif
static struct msm_fb_panel_data hx8369b_panel_data = {
	.on	= mipi_hx8369b_lcd_on,
	.off = mipi_hx8369b_lcd_off,
	#if defined(CONFIG_BACKLIGHT_IC_KTD3102)
	.set_backlight = mipi_hx8369b_disp_backlight,
	#else
	.set_backlight = mipi_hx8369b_set_backlight,
	#endif
};  

static int ch_used[3];

static int mipi_hx8369b_lcd_init(void)
{
	mipi_dsi_buf_alloc(&hx8369b_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&hx8369b_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}
int mipi_hx8369b_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	printk(KERN_ERR "[LCD] mipi_hx8369b_device_register\n");
	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_hx8369b_lcd_init();
	if (ret) {
		pr_err("mipi_hx8369b_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_hx8369b", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	hx8369b_panel_data.panel_info = *pinfo;
	ret = platform_device_add_data(pdev, &hx8369b_panel_data,
				sizeof(hx8369b_panel_data));
	if (ret) {
		pr_debug("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_debug("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}
