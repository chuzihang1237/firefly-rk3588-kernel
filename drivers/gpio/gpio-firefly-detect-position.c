#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h> /* for enum gpiod_flags */
#include <linux/sysfs.h>

struct firefly_position_info {
    int     b_gpio[7]; // B0->B6 对应下标0->6
    int     a_gpio[4]; // A0->A3 对应下标0->3
};

static struct firefly_position_info *info;

struct board_position_info {
    char*   position_name;
    int     position_value;
};

struct board_info {
    char*   board_name;
    int     board_value;
    struct board_position_info* sub_position;
};

static struct board_position_info b1_sub_position[] = {
    {"sub01",0x00},
    {"sub02",0x01},
    {"sub03",0x02},
    {"sub04",0x03},
    {"sub05",0x04},
    {"sub06",0x05},
    {"sub07",0x06},
    {"sub08",0x07},
    {"sub09",0x08},
    {"sub10",0x09},
    {NULL,0xff},
};

// static struct board_position_info b2_sub_position[] = {
//     {"sub1-01",0x00},
//     {"sub1-02",0x00},
//     {"sub1-03",0x00},
//     {"sub1-04",0x00},
//     {"sub1-05",0x00},
//     {"sub1-06",0x00},
//     {"sub1-07",0x00},
//     {"sub1-08",0x00},

//     {"sub2-01",0x00},
//     {"sub2-02",0x00},
//     {"sub2-03",0x00},
//     {"sub2-04",0x00},
//     {"sub2-05",0x00},
//     {"sub2-06",0x00},
//     {"sub2-07",0x00},
//     {"sub2-08",0x00},

//     {"sub3-01",0x00},
//     {"sub3-02",0x00},
//     {"sub3-03",0x00},
//     {"sub3-04",0x00},
//     {"sub3-05",0x00},
//     {"sub3-06",0x00},
//     {"sub3-07",0x00},
//     {"sub3-08",0x00},

//     {"sub4-01",0x00},
//     {"sub4-02",0x00},
//     {"sub4-03",0x00},
//     {"sub4-04",0x00},
//     {"sub4-05",0x00},
//     {"sub4-06",0x00},
//     {"sub4-07",0x00},
//     {"sub4-08",0x00},

//     {"sub5-01",0x00},
//     {"sub5-02",0x00},
//     {"sub5-03",0x00},
//     {"sub5-04",0x00},
//     {"sub5-05",0x00},
//     {"sub5-06",0x00},
//     {"sub5-07",0x00},
//     {"sub5-08",0x00},

//     {"sub6-01",0x00},
//     {"sub6-02",0x00},
//     {"sub6-03",0x00},
//     {"sub6-04",0x00},
//     {"sub6-05",0x00},
//     {"sub6-06",0x00},
//     {"sub6-07",0x00},
//     {"sub6-08",0x00},

//     {"sub7-01",0x00},
//     {"sub7-02",0x00},
//     {"sub7-03",0x00},
//     {"sub7-04",0x00},
//     {"sub7-05",0x00},
//     {"sub7-06",0x00},
//     {"sub7-07",0x00},
//     {"sub7-08",0x00},

//     {"sub8-01",0x00},
//     {"sub8-02",0x00},
//     {"sub8-03",0x00},
//     {"sub8-04",0x00},
//     {"sub8-05",0x00},
//     {"sub8-06",0x00},
//     {"sub8-07",0x00},
//     {"sub8-08",0x00},

//     {"sub9-01",0x00},
//     {"sub9-02",0x00},
//     {"sub9-03",0x00},
//     {"sub9-04",0x00},
//     {"sub9-05",0x00},
//     {"sub9-06",0x00},
//     {"sub9-07",0x00},
//     {"sub9-08",0x00},
//     {NULL,0xff},
// };

static struct board_info position_table[] = {
    {"CS-B1 V1.1", 0x0e, b1_sub_position},
    {"CS-B1 V1.0", 0x0f, b1_sub_position},
    {NULL,0xff,NULL},
};

static struct class *class = NULL;
static struct device *position = NULL;

static ssize_t value_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int position_value = 0;
    int board_value = 0;
    struct board_info *current_board = NULL;
    int i;

    for (i = 0; i < 4; i++) {
        int val = gpio_get_value(info->a_gpio[i]);
        // printk("a_gpio[%d] : %d \n",info->a_gpio[i],val);
        board_value |= (val ? 1 : 0) << i;
    }

    // printk("board_value : %d \n",board_value);
    for (i = 0; position_table[i].board_name != NULL; i++) {
        if (position_table[i].board_value == board_value) {
            current_board = &position_table[i];
            break;
        }
    }

    if (!current_board)
        return sprintf(buf, "unkown\n");

    for (i = 0; i < 7; i++) {
        int val = gpio_get_value(info->b_gpio[i]);
        // printk("b_gpio[%d] : %d \n",info->a_gpio[i],val);
        position_value |= (val ? 1 : 0) << i;
    }

    // printk("position_value : %d \n",position_value);
    struct board_position_info *sub = current_board->sub_position;
    for (i = 0; sub[i].position_name != NULL; i++) {
        if (sub[i].position_value == position_value) {
            return sprintf(buf, "%s\n", sub[i].position_name);
        }
    }

    return sprintf(buf, "unkown\n");
}
static DEVICE_ATTR(value, S_IRUGO, value_show, NULL);

static ssize_t board_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int board_value = 0;
    int i;

    for (i = 0; i < 4; i++) {
        int val = gpio_get_value(info->a_gpio[i]);
        board_value |= (val ? 1 : 0) << i;
    }

    for (i = 0; position_table[i].board_name != NULL; i++) {
        if (position_table[i].board_value == board_value) {
            return sprintf(buf, "%s\n", position_table[i].board_name);
        }
    }

    return sprintf(buf, "unkown\n");
}
static DEVICE_ATTR(board, S_IRUGO, board_show, NULL);

static int firefly_detect_position_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct pinctrl *pinctrl;
    int ret;
    int i;

    info = devm_kzalloc(dev, sizeof(*info), GFP_KERNEL);
    if (!info)
        return -ENOMEM;

    platform_set_drvdata(pdev, info);

    pinctrl = devm_pinctrl_get_select_default(dev);
    if (IS_ERR(pinctrl)) {
        dev_err(dev, "Failed to get pinctrl\n");
        return PTR_ERR(pinctrl);
    }

    const char *a_gpio_names[] = {
        "A0-gpio", "A1-gpio",
        "A2-gpio", "A3-gpio"
    };
    for (i = 0; i < 4; i++) {
        enum of_gpio_flags flags;
        int gpio = of_get_named_gpio_flags(dev->of_node, a_gpio_names[i], 0, &flags);
        
        if (gpio < 0) {
            dev_err(dev, "Failed to get A%d gpio\n", i);
            return gpio;
        }
        info->a_gpio[i] = gpio;

        ret = devm_gpio_request(dev, gpio, dev_name(dev));
        if (ret) {
            dev_err(dev, "Failed to request A%d gpio%d\n", i, gpio);
            return ret;
        }
        gpio_direction_input(gpio);
    }

    const char *b_gpio_names[] = {
        "B0-gpio", "B1-gpio", "B2-gpio",
        "B3-gpio", "B4-gpio", "B5-gpio",
        "B6-gpio"
    };
    for (i = 0; i < 7; i++) {
        enum of_gpio_flags flags;
        int gpio = of_get_named_gpio_flags(dev->of_node, b_gpio_names[i], 0, &flags);
        
        if (gpio < 0) {
            dev_err(dev, "Failed to get B%d gpio\n", i);
            return gpio;
        }
        info->b_gpio[i] = gpio;

        ret = devm_gpio_request(dev, gpio, dev_name(dev));
        if (ret) {
            dev_err(dev, "Failed to request B%d gpio%d\n", i, gpio);
            return ret;
        }
        gpio_direction_input(gpio);
    }

    class = class_create(THIS_MODULE, "sub_position");
    if (IS_ERR(class)) {
        dev_err(dev, "failed to create class sub_position\n");
        return PTR_ERR(class);
    }

    position = device_create(class, NULL, MKDEV(0, 0), NULL, "position");
    if (IS_ERR(position)) {
        dev_err(dev, "failed to create device position\n");
        class_destroy(class);
        return PTR_ERR(position);
    }

    ret = device_create_file(position, &dev_attr_value);
    if (ret) {
        dev_err(dev, "failed to create attribute value \n");
        device_destroy(class, MKDEV(0, 0));
        class_destroy(class);
        return ret;
    }

    ret = device_create_file(position, &dev_attr_board);
    if (ret) {
        dev_err(dev, "failed to create attribute board \n");
        device_destroy(class, MKDEV(0, 0));
        class_destroy(class);
        return ret;
    }

    dev_info(dev, "detection GPIOs initialized\n");
    return 0;
}

static struct of_device_id firefly_match_table[] = {
    { .compatible = "firefly,detect-position-gpio",},
    {},
};

static struct platform_driver firefly_detect_position_driver = {
    .driver = {
            .name = "firefly-detect-position",
            .owner = THIS_MODULE,
            .of_match_table = firefly_match_table,
    },
    .probe = firefly_detect_position_probe,
};

static int firefly_detect_position_init(void)
{
    int ret;
    printk("Firefly detect position driver init!\n");
    ret = platform_driver_register(&firefly_detect_position_driver);
    printk("Firefly detect position driver register return : %d \n",ret);
    return ret ;
}
module_init(firefly_detect_position_init);

static void firefly_detect_position_exit(void)
{
    platform_driver_unregister(&firefly_detect_position_driver);
}
module_exit(firefly_detect_position_exit);

MODULE_AUTHOR("maocl <service@t-firefly.com>");
MODULE_DESCRIPTION("Firefly Detect Position driver");
MODULE_ALIAS("platform:firefly-detect-position");
MODULE_LICENSE("GPL");