load(":target_variants.bzl", "la_variants")
load(":msm_kernel_la.bzl", "define_msm_la")
load(":image_opts.bzl", "boot_image_opts")

target_name = "sun"

def define_sun():
    _sun_in_tree_modules = [
        # keep sorted
        "drivers/clk/qcom/camcc-sun.ko",
        "drivers/clk/qcom/clk-dummy.ko",
        "drivers/clk/qcom/clk-qcom.ko",
        "drivers/clk/qcom/evacc-sun.ko",
        "drivers/clk/qcom/gcc-sun.ko",
        "drivers/clk/qcom/gdsc-regulator.ko",
        "drivers/firmware/qcom-scm.ko",
        "drivers/interconnect/icc-test.ko",
        "drivers/interconnect/qcom/icc-bcm-voter.ko",
        "drivers/interconnect/qcom/icc-debug.ko",
        "drivers/interconnect/qcom/icc-rpmh.ko",
        "drivers/interconnect/qcom/qnoc-qos.ko",
        "drivers/interconnect/qcom/qnoc-sun.ko",
        "drivers/iommu/arm/arm-smmu/arm_smmu.ko",
        "drivers/mailbox/qcom-ipcc.ko",
        "drivers/pinctrl/qcom/pinctrl-msm.ko",
        "drivers/pinctrl/qcom/pinctrl-sun.ko",
        "drivers/power/reset/qcom-reboot-reason.ko",
        "drivers/regulator/debug-regulator.ko",
        "drivers/regulator/proxy-consumer.ko",
        "drivers/regulator/stub-regulator.ko",
        "drivers/soc/qcom/llcc-qcom.ko",
        "drivers/soc/qcom/secure_buffer.ko",
        "drivers/soc/qcom/smem.ko",
        "drivers/soc/qcom/socinfo.ko",
    ]

    _sun_consolidate_in_tree_modules = _sun_in_tree_modules + [
        # keep sorted
        "drivers/misc/lkdtm/lkdtm.ko",
        "kernel/locking/locktorture.ko",
        "kernel/rcu/rcutorture.ko",
        "kernel/torture.ko",
        "lib/atomic64_test.ko",
        "lib/test_user_copy.ko",
    ]

    for variant in la_variants:
        if variant == "consolidate":
            mod_list = _sun_consolidate_in_tree_modules
        else:
            mod_list = _sun_in_tree_modules

        define_msm_la(
            msm_target = target_name,
            variant = variant,
            in_tree_module_list = mod_list,
            boot_image_opts = boot_image_opts(
                earlycon_addr = "qcom_geni,0x00a9c000",
                kernel_vendor_cmdline_extras = [
                    # do not sort
                    "console=ttyMSM0,115200n8",
                    "nowatchdog",  # disable wdog for now
                    "qcom_geni_serial.con_enabled=1",
                    "bootconfig",
                    "printk.devkmsg=on",
                    "loglevel=8",
                    "nokaslr",
                    "androidboot.first_stage_console=1",
                ],
            ),
        )
