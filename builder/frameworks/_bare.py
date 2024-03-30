"""
Default flags for bare-metal programming (without any framework layers)
"""

import os
from common import *

march = get_board_config("build.march", "rv32imafc")
mabi  = get_board_config("build.mabi", "ilp32f")
mcmodel = get_board_config("build.mcmodel", "medany")

build_debug = get_project_option("build_type", "release").lower() == "debug"
"""
debug_build_flags = []
Settings here will be overwritten internally to debug_build_flags's default value.
Set the debug_build_flags's default value to this in platform.py instead.
debug_build_flags = get_project_options("debug_build_flags", "-g3 -ggdb3 -gdwarf-4".split(" "))
"""
from platformio.project.options import ProjectOptions
debug_build_flags = get_project_options("debug_build_flags", ProjectOptions["env.debug_build_flags"].default.split(" "))
#debug_build_flags = get_project_options("debug_build_flags", "-g3 -ggdb3 -gdwarf-4")
release_build_flags = get_project_options("release_build_flags", "-Os")

env.Append(
    ASFLAGS=[
        "-c",
        "-x", "assembler-with-cpp",
    ],

    CFLAGS=[
        "-Werror=implicit-function-declaration"
    ],
    CCFLAGS=[
        "-fdata-sections",
        "-ffunction-sections",
        "-march=%s" % march,
        "-mabi=%s" % mabi,
        "-mcmodel=%s" %  mcmodel
    ],

    LINKFLAGS=[
        "-fdata-sections",
        "-ffunction-sections",
        "-fsingle-precision-constant",
        "-march=%s" % march,
        "-mabi=%s" % mabi,
        "-mcmodel=%s" %  mcmodel,
        "-static",
        "-nostartfiles",
        "-Wl,--gc-sections"
    ],

    CPPDEFINES=[
        ("BOARD", "$BOARD")
    ],

    CPPPATH=[
#       join(pio.get_dir(), "boards"),
#       "$PROJECT_SRC_DIR",
#       "$PROJECT_INCLUDE_DIR"
    ],

    LIBPATH=[
    ],

    LIBS=["c", "m", "gcc"],
)

opt_flags = debug_build_flags if build_debug else release_build_flags
env.Append(
    CCFLAGS=opt_flags,
    LINKFLAGS=opt_flags
)

# copy CCFLAGS to ASFLAGS (-x assembler-with-cpp mode)
env.Append(ASFLAGS=env.get("CCFLAGS", [])[:])
