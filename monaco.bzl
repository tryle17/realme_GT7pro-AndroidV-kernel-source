load(":target_variants.bzl", "la_variants")
load(":msm_kernel_la.bzl", "define_msm_la")
load(":image_opts.bzl", "boot_image_opts")

target_name = "monaco"

def define_monaco():
    _monaco_in_tree_modules = [
        # keep sorted
        "drivers/char/rdbg.ko",
        "drivers/clk/qcom/clk-dummy.ko",
        "drivers/clk/qcom/clk-qcom.ko",
        "drivers/cpufreq/qcom-cpufreq-hw.ko",
        "drivers/cpuidle/governors/qcom_lpm.ko",
        "drivers/crypto/qcom-rng.ko",
        "drivers/dma-buf/heaps/qcom_dma_heaps.ko",
        "drivers/dma/qcom/msm_gpi.ko",
        "drivers/firmware/qcom-scm.ko",
        "drivers/hwspinlock/qcom_hwspinlock.ko",
        "drivers/i2c/busses/i2c-msm-geni.ko",
        "drivers/mfd/qcom-i2c-pmic.ko",
        "drivers/mfd/qcom-spmi-pmic.ko",
        "drivers/mmc/host/cqhci.ko",
        "drivers/mmc/host/sdhci-msm.ko",
        "drivers/phy/qualcomm/phy-qcom-ufs.ko",
        "drivers/pinctrl/qcom/pinctrl-msm.ko",
        "drivers/power/reset/qcom-dload-mode.ko",
        "drivers/power/reset/qcom-pon.ko",
        "drivers/power/reset/qcom-reboot-reason.ko",
        "drivers/power/reset/reboot-mode.ko",
        "drivers/soc/qcom/boot_stats.ko",
        "drivers/soc/qcom/cpu_phys_log_map.ko",
        "drivers/soc/qcom/dcc_v2.ko",
        "drivers/soc/qcom/debug_symbol.ko",
        "drivers/soc/qcom/eud.ko",
        "drivers/soc/qcom/mem_buf/mem_buf.ko",
        "drivers/soc/qcom/mem_buf/mem_buf_dev.ko",
        "drivers/soc/qcom/memshare/heap_mem_ext_v01.ko",
        "drivers/soc/qcom/memshare/msm_memshare.ko",
        "drivers/soc/qcom/qcom_aoss.ko",
        "drivers/soc/qcom/qcom_cpu_vendor_hooks.ko",
        "drivers/soc/qcom/qcom_ice.ko",
        "drivers/soc/qcom/qcom_logbuf_boot_log.ko",
        "drivers/soc/qcom/qcom_logbuf_vendor_hooks.ko",
        "drivers/soc/qcom/qcom_wdt_core.ko",
        "drivers/soc/qcom/qmi_helpers.ko",
        "drivers/thermal/qcom/thermal_config.ko",
        "drivers/usb/phy/phy-msm-snps-hs.ko",
        "kernel/msm_sysstats.ko",
        "kernel/trace/qcom_ipc_logging.ko",
    ]

    _monaco_consolidate_in_tree_modules = _monaco_in_tree_modules + [
        # keep sorted
        "kernel/locking/locktorture.ko",
        "kernel/rcu/rcutorture.ko",
        "kernel/torture.ko",
    ]

    kernel_vendor_cmdline_extras = [
        # do not sort
        "console=ttyMSM0,115200n8",
        "qcom_geni_serial.con_enabled=1",
        "bootconfig",
    ]

    for variant in la_variants:
        board_kernel_cmdline_extras = []
        board_bootconfig_extras = []

        if variant == "consolidate":
            mod_list = _monaco_consolidate_in_tree_modules
        else:
            mod_list = _monaco_in_tree_modules
            board_kernel_cmdline_extras += ["nosoftlockup"]
            kernel_vendor_cmdline_extras += ["nosoftlockup"]
            board_bootconfig_extras += ["androidboot.console=0"]

        define_msm_la(
            msm_target = target_name,
            variant = variant,
            in_tree_module_list = mod_list,
            boot_image_opts = boot_image_opts(
                earlycon_addr = "qcom_geni,0x04a98000",
                kernel_vendor_cmdline_extras = kernel_vendor_cmdline_extras,
                board_kernel_cmdline_extras = board_kernel_cmdline_extras,
                board_bootconfig_extras = board_bootconfig_extras,
            ),
        )
