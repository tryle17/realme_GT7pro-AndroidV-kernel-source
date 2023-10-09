load(":target_variants.bzl", "la_variants")
load(":msm_kernel_la.bzl", "define_msm_la")
load(":image_opts.bzl", "boot_image_opts")

target_name = "pineapple"

def define_pineapple():
    _pineapple_in_tree_modules = [
        # keep sorted
        "drivers/clk/qcom/camcc-pineapple.ko",
        "drivers/clk/qcom/clk-dummy.ko",
        "drivers/clk/qcom/clk-qcom.ko",
        "drivers/clk/qcom/clk-rpmh.ko",
        "drivers/clk/qcom/debugcc-pineapple.ko",
        "drivers/clk/qcom/dispcc-pineapple.ko",
        "drivers/clk/qcom/gcc-pineapple.ko",
        "drivers/clk/qcom/gdsc-regulator.ko",
        "drivers/clk/qcom/gpucc-pineapple.ko",
        "drivers/clk/qcom/tcsrcc-pineapple.ko",
        "drivers/clk/qcom/videocc-pineapple.ko",
        "drivers/dma-buf/heaps/qcom_dma_heaps.ko",
        "drivers/firmware/arm_scmi/qcom_scmi_vendor.ko",
        "drivers/firmware/qcom-scm.ko",
        "drivers/hwspinlock/qcom_hwspinlock.ko",
        "drivers/interconnect/qcom/icc-bcm-voter.ko",
        "drivers/interconnect/qcom/icc-debug.ko",
        "drivers/interconnect/qcom/icc-rpmh.ko",
        "drivers/interconnect/qcom/qnoc-pineapple.ko",
        "drivers/interconnect/qcom/qnoc-qos.ko",
        "drivers/iommu/arm/arm-smmu/arm_smmu.ko",
        "drivers/iommu/iommu-logger.ko",
        "drivers/iommu/msm_dma_iommu_mapping.ko",
        "drivers/iommu/qcom_iommu_debug.ko",
        "drivers/iommu/qcom_iommu_util.ko",
        "drivers/irqchip/qcom-pdc.ko",
        "drivers/mailbox/msm_qmp.ko",
        "drivers/mailbox/qcom-ipcc.ko",
        "drivers/mfd/qcom-spmi-pmic.ko",
        "drivers/nvmem/nvmem_qfprom.ko",
        "drivers/perf/qcom_llcc_pmu.ko",
        "drivers/phy/qualcomm/phy-qcom-ufs.ko",
        "drivers/phy/qualcomm/phy-qcom-ufs-qmp-v4-pineapple.ko",
        "drivers/pinctrl/qcom/pinctrl-msm.ko",
        "drivers/pinctrl/qcom/pinctrl-pineapple.ko",
        "drivers/regulator/debug-regulator.ko",
        "drivers/regulator/proxy-consumer.ko",
        "drivers/regulator/rpmh-regulator.ko",
        "drivers/remoteproc/qcom_common.ko",
        "drivers/remoteproc/qcom_pil_info.ko",
        "drivers/remoteproc/qcom_q6v5.ko",
        "drivers/remoteproc/qcom_q6v5_pas.ko",
        "drivers/rpmsg/qcom_glink.ko",
        "drivers/rpmsg/qcom_glink_smem.ko",
        "drivers/rpmsg/qcom_glink_spss.ko",
        "drivers/soc/qcom/cmd-db.ko",
        "drivers/soc/qcom/cpucp_log.ko",
        "drivers/soc/qcom/crm.ko",
        "drivers/soc/qcom/dcvs/bwmon.ko",
        "drivers/soc/qcom/dcvs/c1dcvs_scmi_v2.ko",
        "drivers/soc/qcom/dcvs/cpufreq_stats_scmi_v2.ko",
        "drivers/soc/qcom/dcvs/dcvs_fp.ko",
        "drivers/soc/qcom/dcvs/memlat.ko",
        "drivers/soc/qcom/dcvs/mpam.ko",
        "drivers/soc/qcom/dcvs/qcom-dcvs.ko",
        "drivers/soc/qcom/dcvs/qcom-pmu-lib.ko",
        "drivers/soc/qcom/dcvs/qcom_scmi_client.ko",
        "drivers/soc/qcom/llcc-qcom.ko",
        "drivers/soc/qcom/mdt_loader.ko",
        "drivers/soc/qcom/mem_buf/mem_buf.ko",
        "drivers/soc/qcom/mem_buf/mem_buf_dev.ko",
        "drivers/soc/qcom/qcom_aoss.ko",
        "drivers/soc/qcom/qcom_cpucp.ko",
        "drivers/soc/qcom/qcom_rpmh.ko",
        "drivers/soc/qcom/secure_buffer.ko",
        "drivers/soc/qcom/smem.ko",
        "drivers/soc/qcom/smp2p.ko",
        "drivers/soc/qcom/socinfo.ko",
        "drivers/spmi/spmi-pmic-arb.ko",
        "drivers/ufs/host/ufs_qcom.ko",
        "drivers/usb/dwc3/dwc3-msm.ko",
        "drivers/usb/gadget/function/usb_f_ccid.ko",
        "drivers/usb/gadget/function/usb_f_cdev.ko",
        "drivers/usb/gadget/function/usb_f_gsi.ko",
        "drivers/usb/gadget/function/usb_f_qdss.ko",
        "drivers/usb/phy/phy-msm-snps-eusb2.ko",
        "drivers/usb/phy/phy-msm-ssusb-qmp.ko",
        "drivers/usb/phy/phy-qcom-emu.ko",
        "drivers/usb/repeater/repeater.ko",
        "drivers/usb/repeater/repeater-qti-pmic-eusb2.ko",
        "net/qrtr/qrtr.ko",
        "net/qrtr/qrtr-smd.ko",
        "net/qrtr/qrtr-tun.ko",
    ]

    _pineapple_consolidate_in_tree_modules = _pineapple_in_tree_modules + [
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
            mod_list = _pineapple_consolidate_in_tree_modules
        else:
            mod_list = _pineapple_in_tree_modules

        define_msm_la(
            msm_target = target_name,
            variant = variant,
            in_tree_module_list = mod_list,
            boot_image_opts = boot_image_opts(
                earlycon_addr = "qcom_geni,0x0089C000",
                kernel_vendor_cmdline_extras = [
                    # do not sort
                    "console=ttyMSM0,115200n8",
                    "qcom_geni_serial.con_enabled=1",
                    "bootconfig",
                    "printk.devkmsg=on",
                    "loglevel=8",
                    "androidboot.first_stage_console=1",
                ],
            ),
        )
