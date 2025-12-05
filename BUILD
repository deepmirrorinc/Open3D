package(default_visibility = ["//visibility:public"])

cc_library(
    name = "open3d",
    srcs = select({
        "@dm_bazel_platforms//platforms:linux_arm64": [
            "lib/aarch64/libOpen3D.so"
        ],
        "//conditions:default": [
            "lib/x86_64/libOpen3D.so"
        ],
    }),
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
