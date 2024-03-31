"""
Agm32 SDK Framework

Software for Developing on the Agm32 Platform
"""

import os, sys
from common import *
import hashlib
from platformio.project.options import ProjectOptions

env.Replace(SDK_IMPORTED=True)

logic_ips = ips if need_prelogic else []
lib_ips   = [] if need_prelogic or need_preip else ips

if lib_ips:
    frameworks.append("agrv_ips")

build_debug = get_project_option("build_type", "release").lower() == "debug"
logger_if = get_project_option("logger_if", "CUSTOM").upper()
monitor_speed = get_project_option("monitor_speed", ProjectOptions["env.monitor_speed"].default)

ips_dir      = get_required_package_dir("framework-agrv_ips")
sdk_dir      = get_required_package_dir("framework-agrv_sdk")
lwip_dir     = get_required_package_dir("framework-agrv_lwip")
tinyusb_dir  = get_required_package_dir("framework-agrv_tinyusb")
freertos_dir = get_required_package_dir("framework-agrv_freertos")
rtthread_dir = get_required_package_dir("framework-agrv_rtthread")
ucos_dir     = get_required_package_dir("framework-agrv_ucos")
embos_dir    = get_required_package_dir("framework-agrv_embos")

lwip_imp_dir     = get_project_option("lwip_imp_dir"    , "")
tinyusb_imp_dir  = get_project_option("tinyusb_imp_dir" , "")
freertos_imp_dir = get_project_option("freertos_imp_dir", "")
rtthread_imp_dir = get_project_option("rtthread_imp_dir", "")
ucos_imp_dir     = get_project_option("ucos_imp_dir"    , "")
embos_imp_dir    = get_project_option("embos_imp_dir"   , "")
lwip_imp_dir     = project_inc_dir if not    lwip_imp_dir  else join(project_dir,     lwip_imp_dir)
tinyusb_imp_dir  = project_inc_dir if not  tinyusb_imp_dir else join(project_dir,  tinyusb_imp_dir)
freertos_imp_dir = project_inc_dir if not freertos_imp_dir else join(project_dir, freertos_imp_dir)
rtthread_imp_dir = project_inc_dir if not rtthread_imp_dir else join(project_dir, rtthread_imp_dir)
ucos_imp_dir     = project_inc_dir if not     ucos_imp_dir else join(project_dir,     ucos_imp_dir)
embos_imp_dir    = project_inc_dir if not     ucos_imp_dir else join(project_dir,    embos_imp_dir)

env.SConscript("_bare.py", exports="env")


env.Append(
    ASFLAGS=[
    ],
    CCFLAGS=[
    ],
    LINKFLAGS=[
    ],
    CPPDEFINES=[
    ],
    CPPPATH=[
        "$BUILD_DIR"
    ],
    LIBPATH=[
    #   "$BUILD_DIR"
    ],
    LIBS=[
    ],
)

def build_board_libs():
    global boot_addr, boot_mode

    libs = []

    env.Append(
        ASFLAGS=[
        ],
        CCFLAGS=[
        ],
        LINKFLAGS=[
        ],
        CPPDEFINES=[
        ],
        CPPPATH=[
            board_dir,
        ],
        LIBPATH=[
        ],
        LIBS=[
        ]
    )
    
    if build_debug:
        env.Append(CPPDEFINES=["BOARD_DEBUG"])
    else:
        env.Append(CPPDEFINES=["BOARD_NDEBUG"])
    
    default_flags = []
    default_filter = ["+<%s/*.c>"%board_dir]
    
    build_flags  = default_flags + get_package_build_options("board", "flags")
    build_filter = default_filter + get_package_build_options("board", "filter")

    envsafe = env.Clone()
    envsafe.Append(ASFLAGS=[], CCFLAGS=build_flags)
    libs = [
        envsafe.BuildLibrary(
            join("$BUILD_DIR", "boards", board),
            board_dir,
            src_filter=build_filter
        )
    ]

    # Prevent the boards/*.c from being double counted
    if check_relpath(board_dir, project_src_dir):
        env.Append(SRC_FILTER=["-<%s>"%join(board_dir, "*.c")])

    return libs

def build_sdk_libs():
    global boot_addr, boot_mode

    libs = []

    env.Append(
        ASFLAGS=[
        ],
        CCFLAGS=[
        ],
        LINKFLAGS=[
        ],
        CPPDEFINES=[
            "DEFINE_MALLOC",
            "DEFINE_FREE"
        ],
    
        CPPPATH=[
            join(sdk_dir, "misc")
        ],
    
        LIBPATH=[
            join(sdk_dir, "misc"),
            join(sdk_dir, "misc", "devices")
        ],
    
        LIBS=[
        ]
    )

    if build_debug:
        env.Append(CPPDEFINES=["SDK_DEBUG"])
    else:
        env.Append(CPPDEFINES=["SDK_NDEBUG"])
    
    default_flags = ["-Wno-cast-align"]
    default_filter = [
        "-<*>",
        "+<misc/*.c>",
        "+<misc/crt.S>"
    ]

    if "agrv_sdk" not in inline_frameworks:
        env.Append(CPPPATH=[join(sdk_dir, "src")])
        default_filter += ["+<src/*.c>"]
    
    if logger_if == "RTT":
        env.Append(
            CPPDEFINES=["SEGGER_RTT", "LOGGER_RTT",
                        ("SEGGER_RTT_MODE_DEFAULT", "SEGGER_RTT_MODE_NO_BLOCK_TRIM")],
            CPPPATH=[join(sdk_dir, "misc", "segger", "RTT")]
        )
        default_filter += ["+<misc/segger/RTT/SEGGER_RTT.c>"]
    elif logger_if.startswith("UART"):
        env.Append(
            CPPDEFINES=[
                ("LOGGER_UART", logger_if[4:]),
                ("LOGGER_BAUD_RATE", monitor_speed)
            ]
        )
    elif logger_if.startswith("CUSTOM"):
        env.Append(
            CPPDEFINES=[
                "LOGGER_CUSTOM",
                ("LOGGER_BAUD_RATE", monitor_speed)
            ]
        )
    
    if boot_mode not in ("flash", "sram", "flash_sram", "itim", "rom"):
        fatal("Unrecognized boot_mode %s, must be flash, sram or flash_sram\n" % boot_mode)
    else:
        env.Append(CPPDEFINES=["%s_BOOT"%boot_mode.upper()])
        env.Replace(BOOT_MODE=boot_mode)
    
    upload_address = get_board_config("upload.address", "")
    # If upload address is empty, it follows boot_addr if it's a number, or the base address for the given boot mode.
    if not upload_address:
        try:
            int(boot_addr, 0)
            upload_address = boot_addr
        except:
            upload_address = "0x%x" % eval("get_" + boot_mode.split('_')[0] + "_info()")[0]
    env.Replace(UPLOAD_ADDRESS=upload_address)
    
    if boot_addr:
        if boot_addr in ("pic", "PIC", "pie", "PIE"):
            env.Append(CCFLAGS=["-f%s"%boot_addr],
                       ASFLAGS=["-f%s"%boot_addr],
                       LINKFLAGS=["-f%s"%boot_addr])
            boot_addr = upload_address
        else:
            if boot_addr.lower() == "upload":
                boot_addr = upload_address
            env.Append(
                LINKFLAGS=["-Wl,--defsym,__boot_addr=" + boot_addr]
            )
    if build_ram_size := get_build_ram_size():
        env.Append(LINKFLAGS=[f"-Wl,--defsym,SRAM_SIZE={build_ram_size}"])
    if build_stack_size := get_build_stack_size():
        env.Append(LINKFLAGS=[f"-Wl,--defsym,__stack_size={build_stack_size}"])
    
    build_flags  = default_flags + get_package_build_options("sdk", "flags")
    build_filter = default_filter + get_package_build_options("sdk", "filter")
    for cpppath in get_package_build_options("sdk", "cpppath"):
        env.Append(CPPPATH=[join(sdk_dir, cpppath)])

    if not logic_skip:
        # Use the auto generated header to pass board information (like HSE and PLL frequencies)
        if os.path.isfile(logic_h) or check_logic:
            env.Append(CPPDEFINES=[("AGM_BOARD_INFO_H", "\\\"%s\\\"" % logic_h)])
        # This header is not recognized as a dependency by SCons. Use a checksum of ve to do that.
        if os.path.isfile(logic_ve):
            logic_checksum = hashlib.sha1(open(logic_ve, 'rb').read()).hexdigest()
            env.Append(CPPDEFINES=[("AGM_BOARD_CHECKSUM", logic_checksum)])

    envsafe = env.Clone()
    if "agrv_rtthread" in frameworks:
        envsafe.Append(CPPDEFINES=[("AGRV_ENTRY", "rtthread_startup")])
    envsafe.Append(ASFLAGS=[], CCFLAGS=build_flags)
    libs = [
        envsafe.BuildLibrary(
            join("$BUILD_DIR", "agrv_sdk"),
            sdk_dir,
            src_filter=build_filter
        )
    ]

    return libs

def build_ips_logic(ips):
    if not ips:
        return

    env.Append(
        ASFLAGS=[
        ],
        CCFLAGS=[
        ],
        LINKFLAGS=[
        ],
        CPPDEFINES=[
        ],
        CPPPATH=[
        ],
        LIBPATH=[
        ],
        LIBS=[
        ]
    )

    if build_debug:
        env.Append(CPPDEFINES=["IPS_DEBUG"])
    else:
        env.Append(CPPDEFINES=["IPS_NDEBUG"])

    ip_defs = []
    for ip in ips:
        ip_def = "IPS_%s"%ip.upper()
        ip_defs.append(ip_def)

    env.Append(CPPDEFINES=ip_defs)

def build_ips_libs(ips):
    libs = []

    if not ips:
        return []

    build_flags0  = get_package_build_options("ips", "flags")
    build_filter0 = get_package_build_options("ips", "filter")

    env.Append(
        ASFLAGS=[
        ],
        CCFLAGS=[
        ],
        LINKFLAGS=[
        ],
        CPPDEFINES=[
        ],
        CPPPATH=[
        ],
        LIBPATH=[
        ],
        LIBS=[
            "nosys"
        ]
    )

    if build_debug:
        env.Append(CPPDEFINES=["IPS_DEBUG"])
    else:
        env.Append(CPPDEFINES=["IPS_NDEBUG"])

    ip_dirs = []
    ip_defs = []
    ip_cpppaths = []
    for ip in ips:
        ip_dir = get_ip_dir(ip)
        ip_def = "IPS_%s"%ip.upper()
        if not ip_dir:
            error("Missing IP %s definition directory!\n" % ip)
            continue

        ip_dirs.append(ip_dir)
        ip_defs.append(ip_def)
        ip_cpppaths.extend([join(ip_dir, pp) for pp in get_package_build_options("ips_%s"%ip, "cpppath")])
        default_flags = []
        default_filter = ["-<*>" "+<*.c>"]
        build_flags  = default_flags  + build_flags0  + get_package_build_options("ips_%s"%ip, "flags")
        build_filter = default_filter + build_filter0 + get_package_build_options("ips_%s"%ip, "filter")

        envsafe = env.Clone()
        envsafe.Append(ASFLAGS=[], CCFLAGS=build_flags, CPPDEFINES=[ip_def], CPPPATH=[ip_dir])

        libs.append(envsafe.BuildLibrary(
            join("$BUILD_DIR", "ips", ip),
            ip_dir,
            src_filter=build_filter
        ))

    env.Append(CPPDEFINES=ip_defs, CPPPATH=ip_dirs)
    env.Append(CPPPATH=ip_cpppaths)
    return libs

def get_lwip_srcs(header, dir, filelist, exclude_dirs=[], exclude_files=[]):
    fp = open(filelist, 'r', encoding='UTF-8')
    srcs = []
    for ll in fp:
        pp = ll.find(header)
        if pp < 0:
            continue
        is_excluded = False
        for ee in exclude_dirs:
            if ee in dirname(ll):
                is_excluded = True
        for ee in exclude_files:
            if ee in basename(ll):
                is_excluded = True
        if is_excluded:
            continue
        srcs.append(ll[pp:].replace(header, dir).replace(" \\", "").strip())

    return srcs

def build_lwip_libs():
    libs = []

    env.Append(
        ASFLAGS=[
        ],
        CCFLAGS=[
        ],
        LINKFLAGS=[
        ],
        CPPDEFINES=[
        ],
        CPPPATH=[
            lwip_imp_dir,
            join(lwip_dir, "src", "include"),
            join(lwip_dir, "contrib")
        ],
        LIBPATH=[
        ],
        LIBS=[
        ]
    )

    freertos_filter = []
    if lwip_freertos:
        freertos_filter = ["+<%s>"%join(lwip_freertos, "*.c")]
        env.Append(CPPPATH=[join(lwip_freertos, "include")])

    if build_debug:
        env.Append(CPPDEFINES=["LWIP_DEBUG"])
    else:
        env.Append(CPPDEFINES=["LWIP_NDEBUG"])

    srcs = get_lwip_srcs("$(LWIPDIR)", "src", join(lwip_dir, "src", "Filelists.mk")) + \
           get_lwip_srcs("$(CONTRIBDIR)", "contrib", join(lwip_dir, "contrib", "Filelists.mk"), exclude_files=["test.c"])
    default_flags = []
    default_filter = ["-<*>"] + ["+<%s>"%src for src in srcs if "makefsdata" not in src] + freertos_filter
    build_flags  = default_flags  + get_package_build_options("lwip", "flags")
    build_filter = default_filter + get_package_build_options("lwip", "filter")
    for cpppath in get_package_build_options("lwip", "cpppath"):
        env.Append(CPPPATH=[join(lwip_dir, cpppath)])

    envsafe = env.Clone()
    envsafe.Append(ASFLAGS=["-D__ASSEMBLY__"], CCFLAGS=build_flags)
    libs.append(envsafe.BuildLibrary(
        join("$BUILD_DIR", "agrv_lwip"),
        lwip_dir,
        src_filter=build_filter
    ))

    return libs

def build_tinyusb_libs():
    libs = []

    env.Append(
        ASFLAGS=[
        ],
        CCFLAGS=[
        ],
        LINKFLAGS=[
        ],
        CPPDEFINES=[
            ("CFG_TUSB_MCU", "OPT_MCU_AGRV2K"),
        ],

        CPPPATH=[
            tinyusb_imp_dir,
            join(tinyusb_dir, "src"),
            join(tinyusb_dir, "hw")
        ],
        LIBPATH=[
        ],
        LIBS=[
            "nosys"
        ]
    )

    if build_debug:
        env.Append(CPPDEFINES=["TINYUSB_DEBUG"])
    else:
        env.Append(CPPDEFINES=["TINYUSB_NDEBUG"])

    srcs = []
    if tinyusb_lwip:
        env.Append(
            CPPDEFINES=["TINYUSB_LWIP"],
            CPPPATH=[join(tinyusb_dir, "lib", "networking")]
        )
        srcs.append("lib/networking/*.c")
    srcs += [
        "src/*.c",
        "src/common/*.c",
        "src/device/*.c",
        "src/host/*.c",
        "src/portable/ehci/*.c",
        "src/class/*/*_device.c",
        "src/class/*/*_host.c",
        "hw/bsp/board.c",
        "hw/mcu/agm/*c"
    ]
    default_flags  = []
    default_filter = ["-<*>"] + ["+<%s>"%src for src in srcs]
    build_flags  = default_flags  + get_package_build_options("tinyusb", "flags")
    build_filter = default_filter + get_package_build_options("tinyusb", "filter")
    print(f"build_filter: {build_filter}")
    for cpppath in get_package_build_options("tinyusb", "cpppath"):
        env.Append(CPPPATH=[join(tinyusb_dir, cpppath)])

    envsafe = env.Clone()
    envsafe.Append(ASFLAGS=[], CCFLAGS=build_flags)
    libs.append(envsafe.BuildLibrary(
        join("$BUILD_DIR", "agrv_tinyusb"),
        tinyusb_dir,
        src_filter=build_filter
    ))

    return libs

def build_freertos_libs():
    libs = []

    env.Append(
        ASFLAGS=[
        ],
        CCFLAGS=[
        ],
        LINKFLAGS=[
        ],
        CPPDEFINES=[
        ],

        CPPPATH=[
            freertos_imp_dir,
            join(freertos_dir, "include"),
            join(freertos_dir, "portable", "GCC", "RISC-V")
        ],
        LIBPATH=[
        ],
        LIBS=[
        ]
    )

    if build_debug:
        env.Append(CPPDEFINES=["FREERTOS_DEBUG"])
    else:
        env.Append(CPPDEFINES=["FREERTOS_NDEBUG"])

    srcs = [
        "*.c",
        "portable/GCC/*.c",
        "portable/GCC/RISC-V/*.c",
        "portable/GCC/RISC-V/*.S"
    ]
    if freertos_heap:
        srcs += [
            "portable/MemMang/%s.c"%freertos_heap
        ]
    default_flags = ["-Wno-cast-align"]
    default_filter = ["-<*>"] + ["+<%s>"%src for src in srcs]
    build_flags  = default_flags  + get_package_build_options("freertos", "flags")
    build_filter = default_filter + get_package_build_options("freertos", "filter")
    for cpppath in get_package_build_options("freertos", "cpppath"):
        env.Append(CPPPATH=[join(freertos_dir, cpppath)])

    envsafe = env.Clone()
    envsafe.Append(ASFLAGS=[], CCFLAGS=build_flags)
    libs.append(envsafe.BuildLibrary(
        join("$BUILD_DIR", "agrv_freertos"),
        freertos_dir,
        src_filter=build_filter
    ))

    return libs

def get_rtthread_depend(rtconfig):
    depends = []
    with open(rtconfig, 'r', encoding='UTF-8') as fp:
        for line in fp:
            words = [w for w in line.rstrip().split(" ") if w]
            if len(words) != 2:
                continue
            if words[0] == "#define":
                depends.append(words[1])
    return depends

def get_rtthread_component(script, depends, recursive):
    groups = []
    dir = dirname(script)
    if is_file_or_link(script):
        org_cwd = os.getcwd()
        os.chdir(dir)
        try:
            vars = {}
            exec(f"import building\nbuilding.depends={depends}\nfrom building import *\n"+
                   open(basename(script), 'r', encoding='UTF-8').read(), {}, vars)
            sub_scripts = []
            vals = []
            vals.extend(vars.get("objs", []))
            group = vars.get("group", [])
            if type(group) == list:
                vals.extend(group)
            else:
                vals.append(group)
            for val in vals:
                if type(val) != RtGroup:
                    continue
                if val.sconscript:
                    if recursive:
                        sub_scripts.extend(val.src)
                else:
                    val.src = ["%s/%s"%(dir,src) for src in val.src]
                    val.CPPPATH = [(join(dir,cc) if cc else dir) for cc in val.CPPPATH]
                    groups.append(val)
                    depends.extend(val.depend)
            for sub_script in sub_scripts:
                subs = get_rtthread_component(sub_script, depends, recursive)
                for sub in subs:
                    sub.src = ["%s/%s"%(dir,src) for src in sub.src]
                    sub.CPPPATH = [(join(dir,cc) if cc else dir) for cc in sub.CPPPATH]
                groups.extend(subs)
        except Exception as e:
            fatal("%s\n"%str(e))
            pass
        os.chdir(org_cwd)
    else:
        group = RtGroup(dir, ["*.c", "*.s", "*.S"], CPPPATH=[""])
        group.src = ["%s/%s"%(dir,src) for src in group.src]
        group.CPPPATH = [(join(dir,cc) if cc else dir) for cc in group.CPPPATH]
        groups.append(group)
    return groups

def build_rtthread_libs():
    libs = []

    env.Append(
        ASFLAGS=[
        ],
        CCFLAGS=[
        ],
        LINKFLAGS=[
        ],
        CPPDEFINES=[
        ],

        CPPPATH=[
            rtthread_imp_dir,
            join(rtthread_dir, "include"),
            join(rtthread_dir, "components/drivers/include"),
            join(rtthread_dir, "libcpu", "risc-v", "common"),
            join(rtthread_dir, "libcpu", "risc-v", "agrv"),
        ],
        LIBPATH=[
        ],
        LIBS=[
        ]
    )

    if build_debug:
        env.Append(CPPDEFINES=["RTTHREAD_DEBUG"])
    else:
        env.Append(CPPDEFINES=["RTTHREAD_NDEBUG"])

    default_srcs = [
        "src/*.c",
        "libcpu/risc-v/common/*.c",
        "libcpu/risc-v/common/*.S",
        "libcpu/risc-v/agrv/*.c",
        "libcpu/risc-v/agrv/*.S"
    ]
    default_flags = ["-Wno-cast-align"]
    default_filter = ["-<*>"]
    rtconfig = join(rtthread_imp_dir, "rtconfig.h")
    rtconfig_depends = get_rtthread_depend(rtconfig)
    if is_file_or_link(rtconfig):
        depends = list(set(rtthread_depends+rtconfig_depends))
    groups = []
    org_cwd = os.getcwd()
    os.chdir(join(rtthread_dir, "components"))

    # Filter out componets that are missing
    components = []
    for component in rtthread_components:
        if not is_dir_or_link(component):
            error("Can not find component %s.\n"%component)
            continue
        components.append(component)

    # Find all component's parent that not listed in component yet
    # Process their groups but not sub_component 
    parent_components = []
    for component in components:
        parent_component = component
        while True:
            parent_component = dirname(parent_component)
            if not parent_component or parent_component == ".":
                break
            if is_file_or_link(join(parent_component, "SConscript")) and \
               not parent_component in components and not parent_component in parent_components:
                parent_components.append(parent_component)
    for component in parent_components:
        groups.extend(get_rtthread_component("%s/%s"%(component,"SConscript"), depends, False))

    for component in components:
        groups.extend(get_rtthread_component("%s/%s"%(component,"SConscript"), depends, True))

    os.chdir(org_cwd)

    for group in groups:
        env.Append(CPPPATH=[join(rtthread_dir, "components", cc) for cc in group.CPPPATH])
       #depends = list(set(depends+group.depend).difference(set(rtconfig_depends)))
        depends = list(set(depends).difference(set(rtconfig_depends)))
        env.Append(CPPDEFINES=depends)
        default_srcs += ["components/%s"%r for r in group.src]
        default_flags += group.ASFLAGS + group.CCFLAGS + group.LINKFLAGS
    default_filter += ["+<%s>"%src for src in default_srcs]
    build_flags  = default_flags  + get_package_build_options("rtthread", "flags")
    build_filter = default_filter + get_package_build_options("rtthread", "filter")
    for cpppath in get_package_build_options("rtthread", "cpppath"):
        env.Append(CPPPATH=[join(rtthread_dir, cpppath)])

    envsafe = env.Clone()
    envsafe.Append(ASFLAGS=[], CCFLAGS=build_flags)
    libs.append(envsafe.BuildLibrary(
        join("$BUILD_DIR", "agrv_rtthread"),
        rtthread_dir,
        src_filter=build_filter
    ))

    return libs

def build_ucos_libs():
    libs = []

    env.Append(
        ASFLAGS=[
        ],
        CCFLAGS=[
        ],
        LINKFLAGS=[
        ],
        CPPDEFINES=[
        ],

        CPPPATH=[
            ucos_imp_dir,
            join(ucos_dir, "uC-CPU"),
            join(ucos_dir, "uC-CPU", "RISC-V", "GCC"),
            join(ucos_dir, "uC-LIB"),
            join(ucos_dir, "uC-OS3", "Source"),
            join(ucos_dir, "uC-OS3", "Ports", "RISC-V", "AGRV", "GCC"),
        ],
        LIBPATH=[
        ],
        LIBS=[
        ]
    )

    if build_debug:
        env.Append(CPPDEFINES=["UCOS_DEBUG"])
    else:
        env.Append(CPPDEFINES=["UCOS_NDEBUG"])

    srcs = [
        "uC-CPU/RISC-V/GCC/*.S",
        "uC-CPU/*.c",
        "uC-LIB/*.c",
        "uC-OS3/Source/*.c",
        "uC-OS3/Ports/RISC-V/AGRV/GCC/*.S",
        "uC-OS3/Ports/RISC-V/AGRV/GCC/*.c"
    ]
    default_flags = ["-Wno-cast-align"]
    default_filter = ["-<*>"] + ["+<%s>"%src for src in srcs]
    build_flags  = default_flags  + get_package_build_options("ucos", "flags")
    build_filter = default_filter + get_package_build_options("ucos", "filter")
    for cpppath in get_package_build_options("ucos", "cpppath"):
        env.Append(CPPPATH=[join(ucos_dir, cpppath)])

    envsafe = env.Clone()
    envsafe.Append(ASFLAGS=[], CCFLAGS=build_flags)
    libs.append(envsafe.BuildLibrary(
        join("$BUILD_DIR", "agrv_ucos"),
        ucos_dir,
        src_filter=build_filter
    ))

    return libs

def build_embos_libs():
    libs = []

    env.Append(
        ASFLAGS=[
        ],
        CCFLAGS=[
        ],
        LINKFLAGS=[
        ],
        CPPDEFINES=[
        ],

        CPPPATH=[
            embos_imp_dir,
            join(embos_dir, "Inc")
        ],
        LIBPATH=[
            join(embos_dir, "Lib")
        ],
        LIBS=[
        ]
    )

    if build_debug:
        env.Append(CPPDEFINES=["EMBOS_DEBUG", "DEBUG=1"], LIBS=["osrv32imac_dp"])
    else:
        env.Append(CPPDEFINES=["EMBOS_NDEBUG"], LIBS=["osrv32imac_r"])

    srcs = [
        "Setup/*.c"
    ]
    default_flags = []
    default_filter = ["-<*>"] + ["+<%s>"%src for src in srcs]
    build_flags  = default_flags  + get_package_build_options("embos", "flags")
    build_filter = default_filter + get_package_build_options("embos", "filter")
    for cpppath in get_package_build_options("embos", "cpppath"):
        env.Append(CPPPATH=[join(embos_dir, cpppath)])

    envsafe = env.Clone()
    envsafe.Append(ASFLAGS=[], CCFLAGS=build_flags)
    libs.append(envsafe.BuildLibrary(
        join("$BUILD_DIR", "agrv_embos"),
        embos_dir,
        src_filter=build_filter
    ))

    return libs


libs = []

if "agrv_sdk" in frameworks:
    libs.extend(build_sdk_libs())

libs.extend(build_board_libs())

if "agrv_freertos" in frameworks:
    libs.extend(build_freertos_libs())

if "agrv_rtthread" in frameworks:
    libs.extend(build_rtthread_libs())

if "agrv_ucos" in frameworks:
    libs.extend(build_ucos_libs())

if "agrv_embos" in frameworks:
    libs.extend(build_embos_libs())

if "agrv_lwip" in frameworks:
    libs.extend(build_lwip_libs())

if "agrv_tinyusb" in frameworks:
    libs.extend(build_tinyusb_libs())

if logic_ips:
    build_ips_logic(logic_ips)

if lib_ips:
    libs.extend(build_ips_libs(lib_ips))

# Have to remove system libs and add it back later
# So we can insert -whole-archive to the package libs
sys_libs = " ".join(["-l%s"%ll for ll in env.get("LIBS")])
env.Replace(LIBS=libs)
env.Prepend(_LIBFLAGS="-Wl,-whole-archive ")
env.Append(_LIBFLAGS=" -Wl,-no-whole-archive "+sys_libs)


# Add project specific flags here
env.Append(CCFLAGS=[], CPPPATH=[logic_dir])

ldscripts = ["AgRV2K_%s.ld" % boot_mode.upper()]
ldscript = get_board_config("build.ldscript", "")
if ldscript and ldscript[-1] == "-":
    ldscripts = []
    ldscript = ldscript[:-1]
if ldscript:
    ldscripts.append(ldscript)
#   env.Replace(LDSCRIPT_PATH=ldscript)

linkflags = env.get("LINKFLAGS", "")
pos = len(linkflags)
for ii,ll in enumerate(linkflags):
    if "-Wl,-T" in ll:
        pos = ii
        break
linkflags0 = []
for ldscript in ldscripts:
    linkflags0 += ["-Wl,-T", ldscript]
linkflags = linkflags[:pos] + linkflags0 + linkflags[pos:]
env.Replace(LINKFLAGS=linkflags)

#env.Append(LINKFLAGS=[join(sdk_dir, "misc", "crt.S")])

rtthread_agrv = join(rtthread_dir, "libcpu", "risc-v", "agrv")
if "agrv_rtthread" in frameworks:
    env.Append(LINKFLAGS=["-L%s"%rtthread_agrv, "-Wl,-T", "rtthread.ld"])

embos_agrv = embos_dir
if "agrv_embos" in frameworks:
    env.Append(LINKFLAGS=["-L%s"%embos_agrv, "-Wl,-T", "embOS.ld"])
