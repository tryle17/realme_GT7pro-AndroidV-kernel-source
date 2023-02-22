load(":sun.bzl", "define_sun")
load("//build:msm_kernel_extensions.bzl", "define_top_level_rules")

def define_msm_platforms():
    define_top_level_rules()
    define_sun()
