load(":target_variants.bzl", "la_variants")
load(":msm_kernel_la.bzl", "define_msm_la")
load(":image_opts.bzl", "boot_image_opts")

target_name = "sun"

def define_sun():
    _sun_in_tree_modules = [
        # keep sorted
        "drivers/bus/mhi/devices/mhi_dev_satellite.ko",
        "drivers/bus/mhi/devices/mhi_dev_uci.ko",
        "drivers/bus/mhi/host/mhi.ko",
        "drivers/clk/qcom/camcc-sun.ko",
        "drivers/clk/qcom/clk-dummy.ko",
        "drivers/clk/qcom/clk-qcom.ko",
        "drivers/clk/qcom/evacc-sun.ko",
        "drivers/clk/qcom/gcc-sun.ko",
        "drivers/clk/qcom/gdsc-regulator.ko",
        "drivers/dma-buf/heaps/qcom_dma_heaps.ko",
        "drivers/firmware/qcom-scm.ko",
        "drivers/hwspinlock/qcom_hwspinlock.ko",
        "drivers/interconnect/icc-test.ko",
        "drivers/interconnect/qcom/icc-bcm-voter.ko",
        "drivers/interconnect/qcom/icc-debug.ko",
        "drivers/interconnect/qcom/icc-rpmh.ko",
        "drivers/interconnect/qcom/qnoc-qos.ko",
        "drivers/interconnect/qcom/qnoc-sun.ko",
        "drivers/iommu/arm/arm-smmu/arm_smmu.ko",
        "drivers/iommu/iommu-logger.ko",
        "drivers/iommu/msm_dma_iommu_mapping.ko",
        "drivers/iommu/qcom_iommu_debug.ko",
        "drivers/iommu/qcom_iommu_util.ko",
        "drivers/mailbox/qcom-ipcc.ko",
        "drivers/mmc/host/cqhci.ko",
        "drivers/mmc/host/sdhci-msm.ko",
        "drivers/nvmem/nvmem_qfprom.ko",
        "drivers/phy/qualcomm/phy-qcom-ufs.ko",
        "drivers/phy/qualcomm/phy-qcom-ufs-qrbtc-sdm845.ko",
        "drivers/pinctrl/qcom/pinctrl-msm.ko",
        "drivers/pinctrl/qcom/pinctrl-sun.ko",
        "drivers/power/reset/qcom-reboot-reason.ko",
        "drivers/regulator/debug-regulator.ko",
        "drivers/regulator/proxy-consumer.ko",
        "drivers/regulator/rpmh-regulator.ko",
        "drivers/regulator/stub-regulator.ko",
        "drivers/remoteproc/qcom_common.ko",
        "drivers/remoteproc/qcom_pil_info.ko",
        "drivers/remoteproc/qcom_q6v5.ko",
        "drivers/remoteproc/qcom_q6v5_pas.ko",
        "drivers/scsi/sg.ko",
        "drivers/soc/qcom/cmd-db.ko",
        "drivers/soc/qcom/llcc-qcom.ko",
        "drivers/soc/qcom/mdt_loader.ko",
        "drivers/soc/qcom/mem_buf/mem_buf.ko",
        "drivers/soc/qcom/mem_buf/mem_buf_dev.ko",
        "drivers/soc/qcom/panel_event_notifier.ko",
        "drivers/soc/qcom/qcom_aoss.ko",
        "drivers/soc/qcom/qcom_ramdump.ko",
        "drivers/soc/qcom/qcom_rpmh.ko",
        "drivers/soc/qcom/secure_buffer.ko",
        "drivers/soc/qcom/smem.ko",
        "drivers/soc/qcom/socinfo.ko",
        "drivers/ufs/host/ufs_qcom.ko",
        "drivers/usb/dwc3/dwc3-msm.ko",
        "drivers/usb/gadget/function/usb_f_ccid.ko",
        "drivers/usb/gadget/function/usb_f_cdev.ko",
        "drivers/usb/gadget/function/usb_f_gsi.ko",
        "drivers/usb/gadget/function/usb_f_qdss.ko",
        "drivers/usb/phy/phy-generic.ko",
        "drivers/usb/phy/phy-qcom-emu.ko",
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
