//
// Created by lili on 2020/8/20.
//
#define DEBUG

#include <common.h>
#include <dm.h>
#include <adc.h>
#include <asm-generic/gpio.h>
#include <dm/device.h>
#include <power/fuel_gauge.h>
#include <linux/usb/phy-rockchip-usb2.h>

struct ip2315_charger_pdata {
    struct gpio_desc     pd;
    u8                   poe_chan;
};

static bool ip2315_charger_status(struct udevice *dev){
    struct ip2315_charger_pdata *pdata = dev_get_platdata(dev);

    if (dm_gpio_is_valid(&pdata->pd)) {
        if(!dm_gpio_get_value(&pdata->pd)){
            debug("fast charge\n");
        }else{
            debug("low charge\n");
        }
    }

    unsigned int adcval;
    if (!adc_channel_single_shot("saradc", pdata->poe_chan, &adcval)) {
        debug("poe get: %d\n", adcval);
        if(adcval >= 800){
            debug("poe 24 charge\n");
            return true;
        }else if(adcval >= 400){
            debug("poe 12 or pd charge\n");
            return true;
        }else if(rockchip_chg_get_type()){
            debug("usb charge\n");
            return true;
        }
    }

    return false;
}

static int ip2315_charger_capability(struct udevice *dev){
    return FG_CAP_CHARGER;
}

static int ip2315_ofdata_to_platdata(struct udevice *dev){
    struct ip2315_charger_pdata *pdata = dev_get_platdata(dev);

    int ret = gpio_request_by_name(dev, "pd-gpio", 0, &pdata->pd, GPIOD_IS_IN);
    if(ret){
        printf("get pd-gpio error!\n");
    }

    u32 chn[2];
    ret = dev_read_u32_array(dev, "io-channels", chn, ARRAY_SIZE(chn));
    if(ret){
        printf("get poe error!\n");
    }

    pdata->poe_chan = chn[1];

    return 0;
}

static int ip2315_probe(struct udevice *dev){
    return 0;
}

static const struct udevice_id charger_ids[] = {
        { .compatible = "ip2315,charger" },
        { },
};

static struct dm_fuel_gauge_ops charger_ops = {
        .get_chrg_online = ip2315_charger_status,
        .capability = ip2315_charger_capability,
};

U_BOOT_DRIVER(ip2315_charger) = {
        .name = "ip2315_charger",
        .id = UCLASS_FG,
        .probe = ip2315_probe,
        .of_match = charger_ids,
        .ops = &charger_ops,
        .ofdata_to_platdata = ip2315_ofdata_to_platdata,
        .platdata_auto_alloc_size = sizeof(struct ip2315_charger_pdata),
};