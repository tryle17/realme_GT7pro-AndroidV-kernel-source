load(":pineapple.bzl", "define_pineapple")
load(":sun.bzl", "define_sun")
load(":msm_common.bzl", "define_signing_keys")
load("//build:msm_kernel_extensions.bzl", "define_top_level_rules")

def define_msm_platforms():
    define_top_level_rules()
    define_signing_keys()
    define_pineapple()
    define_sun()
