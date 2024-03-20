package(default_visibility = ["//visibility:public"])

cc_library(
    name = "open3d",
    srcs = [
        "lib/libOpen3D.so",
    ],
    hdrs = glob(
        [
            "include/open3d/**/*.h",
        ],
    ),
    includes = [
        "include/open3d/3rdparty",
    ],
    strip_include_prefix = "include",
)
