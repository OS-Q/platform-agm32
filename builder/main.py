import os
from common import *

try:
    import psutil
except ImportError:
    env.Execute(
        env.VerboseAction(
            "$PYTHONEXE -m pip install psutil",
            "Installing Agm32 SDK's Python dependencies",
        )
    )

env.SConscript("compat.py", exports="env")
env.SConsignFile(
    os.path.join(
        project_workspace_dir, ".sconsign%d%d" % (sys.version_info[0], sys.version_info[1])
    )
)

import subprocess
python_version_error = False
try:
    python_version0 = 0
    python_version1 = 0
    python_version2 = 0
    python_version = subprocess.run([env.get("PYTHONEXE"), "--version"], capture_output=True)
    python_version = python_version.stdout.decode().split(" ")
    if len(python_version) == 2:
        python_version = python_version[1].strip()
    else:
        python_version = ""
    print("Python version: %s" % python_version)
    python_versions = python_version.split(".")
    if len(python_versions) >= 1:
        python_version0 = int(python_versions[0])
    if len(python_version) >= 2:
        python_version1 = int(python_versions[1])
    if len(python_version) >= 3:
        python_version2 = int(python_versions[2])
    if ((python_version0 > 0 and python_version0 < 3) or
        (python_version0 == 3 and python_version1 < 8)):
        python_version_error = True
except:
    warn("Can not identify Python version.\n")
if python_version_error:
    fatal("Unsupported Python version %s, >= 3.8 required.\n" % python_version)

sdk_dir = get_required_package_dir("framework-agrv_sdk")
toolchain_dir = get_required_package_dir("toolchain-agrv")
toolchain_clang_dir = get_required_package_dir("toolchain-agrv-clang")
jlink_dir = get_required_package_dir("tool-agrv_jlink")
openocd_dir = get_required_package_dir("tool-agrv_openocd")
flasher_dir = get_required_package_dir("tool-agrv_flashloader")

toolchain_bin = join(toolchain_dir, "bin") 
toolchain_clang_bin = join(toolchain_clang_dir, "bin") 
xlen=64
cc, cxx, ar, ranlib = "gcc", "g++", "gcc-ar", "gcc-ranlib"
env.Replace(
    AR=join(toolchain_bin, "riscv%s-unknown-elf-%s" % (xlen,ar)),
    AS=join(toolchain_bin, "riscv%s-unknown-elf-%s" % (xlen,cc)),
    CC=join(toolchain_bin, "riscv%s-unknown-elf-%s" % (xlen,cc)),
    CXX=join(toolchain_bin, "riscv%s-unknown-elf-%s" % (xlen,cxx)),
    LINK=join(toolchain_bin, "riscv%s-unknown-elf-%s" % (xlen,cc)),
    RANLIB=join(toolchain_bin, "riscv%s-unknown-elf-%s" % (xlen,ranlib)),

    GDB=join(toolchain_bin, "riscv%s-unknown-elf-gdb" % xlen),
    OBJCOPY=join(toolchain_bin, "riscv%s-unknown-elf-objcopy" % xlen),
    OBJDUMP=join(toolchain_bin, "riscv%s-unknown-elf-objdump" % xlen),
    READELF=join(toolchain_bin, "riscv%s-unknown-elf-readelf" % xlen),
    SIZETOOL=join(toolchain_bin, "riscv%s-unknown-elf-size" % xlen)
)
env.Replace(
    ARFLAGS = ["rc"],
    SIZEPRINTCMD = '$SIZETOOL -d $SOURCES',
    PROGSUFFIX = ".elf",
    SIZECHECKCMD = "$PYTHONEXE %s $SIZETOOL $SOURCES" % join(module_dir, "size.py"),
    SIZEPROGREGEXP = r"^(\d+)\s+\d+",
    SIZEDATAREGEXP = r"^\d+\s+(\d+)"
)

toolchain_type = get_project_option("toolchain", "gnu")
if toolchain_type == "gnu":
    env.Append(
        CCFLAGS=["-mstrict-align",
                 "-Werror=cast-align",
                 "-fsingle-precision-constant"]
    )
elif toolchain_type == "clang":
    cc, cxx = "cc", "cc"
    env.Append(
        CCFLAGS=["--gcc-toolchain=%s" % toolchain_dir,
                 "--target=riscv64",
                 "-Wno-cast-align"]
    )
    env.Replace(
        CC=join(toolchain_clang_bin, "riscv%s-unknown-elf-%s" % (xlen,cc)),
        CXX=join(toolchain_clang_bin, "riscv%s-unknown-elf-%s" % (xlen,cxx))
    )
else:
    fatal("Unknown toolchain type \'%s\'. Please use one of \'%s\'.\n"
                % (toolchain_type, ", ".join(["gnu", "clang"])))

# Allow user to override via pre:script
program = env.get("PROGNAME", None)
envname = env.get("PIOENV", "default")
if not program or program == "program":
    program = get_project_option("program", "firmware")
    env.Replace(PROGNAME=program)
batch = env.get("BATCHNAME", None)
if not batch:
    batch = get_project_option("batch_output")
if not batch:
    batch = get_project_option("batch", "%s_%s_batch"%(program,envname))
    env.Replace(BATCHNAME=program)

env.Append(
    BUILDERS=dict(
        ElfToHex=Builder(
            action=env.VerboseAction(" ".join([
                "$OBJCOPY",
                "-O",
                "ihex",
                "$SOURCES",
                "$TARGET"
            ]), "Building $TARGET"),
            suffix=".hex"
        ),
        ElfToBin=Builder(
            action=env.VerboseAction(" ".join([
                "$OBJCOPY",
                "-O",
                "binary",
                "$SOURCES",
                "$TARGET"
            ]), "Building $TARGET"),
            suffix=".bin"
        ),
        ObjDump=Builder(
            action=env.VerboseAction(" ".join([
                "$OBJDUMP",
                "-d",
                "$SOURCES",
                ">"
                "$TARGET"
            ]), "Building $TARGET"),
            suffix=".objdump"
        ),
        ReadElf=Builder(
            action=env.VerboseAction(" ".join([
                "$READELF",
                "-a",
                "$SOURCES",
                ">",
                "$TARGET"
            ]), "Building $TARGET"),
            suffix=".readelf"
        )
    )
)

if not frameworks:
    env.SConscript(join("frameworks", "_bare.py"), exports="env")

#
# Target: Build executable and linkable firmware
#

target_elf = None
if "nobuild" in COMMAND_LINE_TARGETS:
    target_elf = join("$BUILD_DIR", "${PROGNAME}.elf")
    target_hex = join("$BUILD_DIR", "${PROGNAME}.hex")
    target_bin = join("$BUILD_DIR", "${PROGNAME}.bin")
    target_objdump = join("$BUILD_DIR", "${PROGNAME}.objdump")
    target_readelf = join("$BUILD_DIR", "${PROGNAME}.readelf")
    program_bin = env.subst(target_bin)
else:
    target_elf = env.BuildProgram()
    target_hex = env.ElfToHex(join("$BUILD_DIR", "${PROGNAME}"), target_elf)
    target_bin = env.ElfToBin(join("$BUILD_DIR", "${PROGNAME}"), target_elf)
    target_objdump = env.ObjDump(join("$BUILD_DIR", "${PROGNAME}"), target_elf)
    target_readelf = env.ReadElf(join("$BUILD_DIR", "${PROGNAME}"), target_elf)
    env.Depends(target_hex, "checkprogsize")
    program_bin = str(target_bin[0])

AlwaysBuild(env.Alias("nobuild", [target_hex, target_bin, target_objdump, target_readelf]))
target_buildprog = env.Alias("buildprog", [target_hex, target_bin, target_objdump, target_readelf])

#
# Target: Print binary size
#

target_size = env.Alias(
    "sizes", target_elf,
    env.VerboseAction("$SIZEPRINTCMD", "Calculating size $SOURCE"))
AlwaysBuild(target_size)


#
# Target: Upload by default .bin file
#

debug = get_board_config("debug", {})
upload_protocols = string_to_list(get_board_config("upload", {}).get("protocols", []))
upload_protocol = env.subst("$UPLOAD_PROTOCOL")
if upload_protocol[-1] == '+': # If protocol ends with +, strip it and don't use env
    upload_protocol = upload_protocol[:-1]
else:
    upload_protocol = get_sys_env("AGM_PIO_UPLOAD_PROTOCOL", upload_protocol)
build_mcu = get_board_config("build", {}).get("mcu", [])
build_device = (build_mcu or "openocd")
upload_target = target_bin
boot_address = get_boot_info()
custom_speed = get_project_option("custom_speed", None)
upload_address, offset_address, upload_start, logic_address = get_upload_info(logic_compress)
upload_speed = get_project_option("upload_speed", None)
upload_speed = str(upload_speed) if upload_speed else upload_speed
boot_version = "0x10"
device_id0, device_id1 = get_device_info()
logic_config  = get_logic_info(logic_address if check_logic>=2 else "0X00000000")

platformio_exe0 = "platformio"
jlink_dbg_exe0 = "JLinkGDBServerCL.exe" if is_win else "JLinkGDBServer"
jlink_exe0 = "JLink.exe" if is_win else "JLinkExe"
openocd_exe0 = "openocd.exe" if is_win else "openocd"

jlink_exe   = join(jlink_dir, jlink_exe0),
openocd_exe = join(openocd_dir, "bin", "openocd_cmd.bat" if is_win else "openocd_cmd"),
flasher_exe = join(flasher_dir, "bin", "agrv32flash_cmd.bat" if is_win else "agrv32flash_cmd")
jc_exe = join(sdk_dir, "etc", "jc"),
oo_exe = join(sdk_dir, "etc", "oo"),
cfg_dir = join(sdk_dir,  "etc")
jc_arg = get_project_option("jc_arg", "")
oo_arg = get_project_option("oo_arg", "")
flasher_arg = get_project_option("flasher_arg", "")

jlink_tool = "-openocd" not in upload_protocol
protocol_base = re.sub("-openocd|-swd|-jtag|-dapdirect_swd", "", upload_protocol.lower())
transport_if = re.sub(f"{protocol_base}|-openocd", "", upload_protocol.lower()).lstrip("-")
if not upload_speed:
    upload_speed = "460800" if protocol_base=="serial" else "auto" if jlink_tool else "10000"
if not custom_speed:
    custom_speed = upload_speed

#
# LOGIC targets
#

supra_dir = get_required_package_dir("tool-agrv_logic")
gen_logic_tcl = join(sdk_dir, "etc", "gen_logic.tcl")
pre_logic_tcl = join(sdk_dir, "etc", "pre_logic.tcl")

def ensure_logic_dir(target, source, env):
    if logic_skip:
        if not is_dir_or_link(logic_dir):
            warn("LOGIC directory %s not found.\n" % logic_dir)
    elif need_prelogic:
        if not is_dir_or_link(logic_dir):
            makedirs(logic_dir)
    else:
        if not is_dir_or_link(project_logic_dir):
            makedirs(project_logic_dir)

def ensure_logic_clean_dir(target, source, env):
    if not logic_skip and not need_prelogic and is_dir_or_link(project_logic_dir):
        fs.rmtree(project_logic_dir)
    ensure_logic_dir(target, source, env)

def ensure_logic_ip_dir(target, source, env):
    if logic_skip:
        if not is_dir_or_link(logic_ip_dir):
            warn("LOGIC directory %s not found.\n" % logic_ip_dir)
    else:
        if not is_dir_or_link(logic_ip_dir):
            makedirs(logic_ip_dir)

def ensure_logic_ip_clean_dir(target, source, env):
    ensure_logic_ip_dir(target, source, env)

batch_exe = join(sdk_dir, "etc", "gen_batch")
vlog_exe = join(sdk_dir, "etc", "gen_vlog")
supra_exe = join(supra_dir, "bin", "af_cmd.bat" if is_win else "af_cmd"),
batch_arg = get_project_option("batch_arg", "")
vlog_arg  = get_project_option("vlog_arg" , "")
supra_arg = get_project_option("logic_arg", "")

check_logic_bin = get_relpath(logic_bin) if (check_logic>=3 and is_file_or_link(logic_bin, check_empty=True)) else None

#
# IP targets
#

ip_vxs = []
ip_sdcs = []
ip_asfs = []
ip_pres = []
ip_posts= []
ip_vvs = []
ip_ve = ""
for ip in ips:
    if logic_skip:
        pass
    elif need_prelogic:
        ip_vv = join(logic_dir, "%s.v"%ip)
        ip_vvs.append(ip_vv)
    else:
        if need_preip:
            logic_ip_ve = join(logic_ip_dir, "%s_.ve"%ip)
        ip_dir = get_ip_dir(ip)
        ip_vx  = join(ip_dir, "%s.vx"%ip)
        ip_vo  = join(ip_dir, "%s.vo"%ip)
        ip_ve = join(ip_dir, "%s.ve"%ip)
        ip_sdc = join(ip_dir, "%s.sdc"%ip)
        ip_asf = join(ip_dir, "%s.asf"%ip)
        ip_pre = join(ip_dir, "%s.pre.asf"%ip)
        ip_post= join(ip_dir, "%s.post.asf"%ip)
        ip_ve = ip_ve if is_file_or_link(ip_ve) else ""
        ip_sdc = ip_sdc if is_file_or_link(ip_sdc) else ""
        ip_asf = ip_asf if is_file_or_link(ip_asf) else ""
        ip_pre = ip_pre if is_file_or_link(ip_pre) else ""
        ip_post= ip_post if is_file_or_link(ip_post) else ""
        if not is_file_or_link(ip_vx) and is_file_or_link(ip_vo):
            ip_vx= ip_vo
        ip_vxs.append(ip_vx)
        ip_sdcs.append(ip_sdc)
        ip_asfs.append(ip_asf)
        ip_pres.append(ip_pre)
        ip_posts.append(ip_post)


#
# Actions
#

upload_actions = []

def validate_upload(source, target, env):
    bin_file = program_bin
    bin_size = getsize(bin_file)
    upload_addr = get_int_value(upload_address)
    address_end = upload_addr+bin_size
    logic_addr = get_int_value(logic_address);
    flash_addr, flash_size = get_flash_info()
    ram_addr, ram_size = get_sram_info()
    itim_addr, itim_size = get_itim_info()
    flash_sizex = logic_addr-flash_addr
    flash_size = min(flash_size, flash_sizex) if flash_sizex>0 else flash_size
    flash_end = flash_addr + flash_size
    ram_end = ram_addr + ram_size
    itim_end = itim_addr + itim_size

    section = check_memory_section(upload_addr)
    if section == "flash" and (address_end >= flash_end or address_end >= logic_addr):
        if (upload_addr >= logic_addr):
            error("Address 0x%x is not a valid FLASH address, it is assigned to LOGIC.\n" % upload_addr)
        elif (address_end >= logic_addr):
            error("Bin file size %d is too large to fit in PROGRAM at 0x%x, maximum is %d.\n" %
                        (bin_size, upload_addr, logic_addr-upload_addr))
        else:
            error("Bin file size %d is too large to fit in FLASH at 0x%x, maximum is %d.\n" %
                        (bin_size, upload_addr, flash_end-upload_addr))
        return -1
    if section == "ram" and address_end >= ram_end:
        error("Bin file size %d is too large to fit in RAM at 0x%x, maximum is %d.\n" %
                    (bin_size, upload_addr, ram_end-upload_addr))
        return -1
    if section == "itim" and address_end >= itim_end:
        error("Bin file size %d is too large to fit in ITIM at 0x%x, maximum is %d.\n" %
                    (bin_size, upload_addr, itim_end-upload_addr))
        return -1
    if section == "other":
        error("Address 0x%x is not a valid FLASH/RAM address.\n" % upload_addr)
        return -1
    return 0

upload_actions += [
        env.VerboseAction(validate_upload,
                          "Checking %s for upload at %s" % (basename(program_bin), upload_address))
]

upload_srst = convert_to_bool(get_project_option("upload_srst", False))
oo_args = [
            "\"%s\""%oo_exe,
            "-d", "\"%s\""%join(openocd_dir, "bin"),
            "-I", build_device,
            "-A", protocol_base,
            "-s", upload_speed,
            "-g", 1
] + (["-R"] if upload_srst else [])

port_action = env.VerboseAction(env.AutodetectUploadPort, "Looking for upload port")

if protocol_base == "serial":
    # UPLOAD_PORT will be updated automatically by AutodetectUploadPort
    def __configure_upload_port(env):
        return env.subst("$UPLOAD_PORT")

    env.Replace(
        __configure_upload_port=__configure_upload_port,
        UPLOADER=flasher_exe,
        UPLOADERFLAGS= [
            "-m", "8e1", "-G", 1,
            "-T", boot_address, "-B", custom_speed, "-X", boot_version,
            "-S", upload_address, "-b", upload_speed,
        ] + (["-g", upload_start] if upload_start else ["-R"])
          + (["-Z"] if (upload_speed!=custom_speed) else [])
          + (["-J", device_id0] if check_device else [])
          + (["-K", logic_config] if check_logic>=1 else [])
          + (["-M", "\"%s\""%check_logic_bin] if check_logic_bin else []),
        UPLOADCMD='"$UPLOADER" $UPLOADERFLAGS %s -v -w "$SOURCE" "${__configure_upload_port(__env__)}"'%(flasher_arg)
    )

    upload_actions += [
        port_action,
        env.VerboseAction("$UPLOADCMD",
            "Uploading $SOURCE to %s @${__configure_upload_port(__env__)}"%upload_address)
    ]

elif upload_protocol in upload_protocols and protocol_base.startswith("jlink") and jlink_tool:
    jlink_device = debug.get("jlink_device")
    env.Replace(
        UPLOADER="$PYTHONEXE",
        UPLOADERFLAGS=[
            "\"%s\""%jc_exe,
            "-d", jlink_device,
            "-a", upload_address, "-s", upload_speed
        ] + (["-j" , transport_if] if transport_if else []),
        UPLOADCMD='"$UPLOADER" $UPLOADERFLAGS %s -w "$SOURCE"'%(jc_arg)
    )

    upload_actions += [
        env.VerboseAction("$UPLOADCMD", "Uploading $SOURCE to %s"%upload_address)
    ]

elif upload_protocol in upload_protocols:
    if offset_address:
        upload_target = target_elf

    address = offset_address or upload_address
    section = check_memory_section(address)
    reg_pc = ("reg pc %s" % address) if section != "flash" else ""
    env.Replace(
        UPLOADER="$PYTHONEXE",
        UPLOADERFLAGS=oo_args+[
            "-a", address
        ] + (["-j" , transport_if] if transport_if else [])
          + (["-J", device_id0] if check_device else [])
          + (["-K", logic_config] if check_logic>=1 else [])
          + (["-M", "\"%s\""%check_logic_bin] if check_logic_bin else [])
          + (["-c", reg_pc, "-t", "2"] if reg_pc else []), # Jump to specified pc, and don't reset after that
        UPLOADCMD='"$UPLOADER" $UPLOADERFLAGS %s -w "$SOURCE"'%(oo_arg)
    )

    upload_actions += [
        env.VerboseAction("$UPLOADCMD", "Uploading $SOURCE to %s"%upload_address)
    ]

# custom upload tool
elif protocol_base == "custom":
    upload_actions += [
        env.VerboseAction("$UPLOADCMD", "Uploading $SOURCE to %s"%upload_address)
    ]

else:
    fatal("Unknown upload protocol \'%s\'. Please use one of \'%s\'.\n"
                % (upload_protocol, ", ".join(upload_protocols)))


target_upload = AlwaysBuild(env.Alias("upload_", upload_target, upload_actions))
AlwaysBuild(env.Alias("upload", (target_buildprog, target_upload)))


logic_actions = []

def validate_logic(source, target, env):
    bin_file = logic_bin
    bin_size = getsize(bin_file)
    logic_addr = get_int_value(logic_address)
    address_end = logic_addr+bin_size
    flash_addr, flash_size = get_flash_info()
    flash_end = flash_addr + flash_size

    section = check_memory_section(logic_address)
    if section == "flash" and address_end >= flash_end:
        error("LOGIC file size %d is too large to fit in FLASH at 0x%x, maximum is %d.\n" %
                    (bin_size, logic_addr, flash_end-logic_addr))
        return -1
    if section == "other":
        error("Address 0x%x is not a valid FLASH address.\n" % logic_addr)
        return -1
    return 0

logic_actions += [
        env.VerboseAction(validate_logic,
                          "Checking %s for LOGIC at %s" % (basename(logic_bin), logic_address))
]

if protocol_base == "serial":
    env.Replace(
        LOGICER=flasher_exe,
        LOGICERFLAGS= [
            "-m", "8e1", "-G", 1,
            "-T", boot_address, "-B", custom_speed, "-X", boot_version,
            "-S", logic_address, "-b", upload_speed, "-R",
        ] + (["-Z"] if (upload_speed!=custom_speed) else [])
          + (["-J", device_id0] if check_device else []), 
        LOGICCMD='"$LOGICER" $LOGICERFLAGS %s -v -l "$SOURCE" "${__configure_upload_port(__env__)}"'%(flasher_arg)
    )
    logic_actions += [
        port_action,
        env.VerboseAction("$LOGICCMD",
            "Uploading $SOURCE to %s @${__configure_upload_port(__env__)}"%logic_address)
    ]
else:
    env.Replace(
        LOGICER="$PYTHONEXE",
        LOGICERFLAGS=oo_args+[
            "-F", logic_address
        ] + (["-j" , transport_if] if transport_if else [])
          + (["-J", device_id0] if check_device else []),
        LOGICCMD='$LOGICER $LOGICERFLAGS %s -f "$SOURCE"'%(oo_arg)
    )
    logic_actions += [
        env.VerboseAction("$LOGICCMD", "Uploading $SOURCE to %s"%logic_address)
    ]

target_buildlogic = env.Alias("buildlogic", abspath(logic_bin))
env.AddCustomTarget(
    name="logic",
    dependencies=abspath(logic_bin),
    actions=logic_actions,
    title="Upload LOGIC",
    description="Programming LOGIC bitstream %s"%basename(logic_bin),
    always_build=True
)
AlwaysBuild(env.Alias("fpga", "logic"))

if not logic_skip:
    vlog_actions = []
    gen_actions = []
    ipvlog_actions = []
    ipgen_actions = []

    ip = ips[0] if ips else ""
    ip_vx  = ip_vxs[0]  if ip_vxs else ""
    ip_sdc = ip_sdcs[0] if ip_sdcs else ""
    ip_asf = ip_asfs[0] if ip_asfs else ""
    ip_pre = ip_pres[0] if ip_pres else ""
    ip_post= ip_posts[0] if ip_posts else ""
    ip_vv  = ip_vvs[0]  if ip_vvs else ""

    os.environ.pop('ALTA_HOME', None)

    if need_prelogic:
        if not ip:
            warn("No IP specified for custom LOGIC.\n")
        env.Replace(
            VLOGER="$PYTHONEXE",
            VLOGERFLAGS=[
                "\"%s\""%vlog_exe, "-c", logic_h
            ],
            VLOGERCMD= \
                    '"$VLOGER" $VLOGERFLAGS %s -s -d "%s" "%s" "%s"'%(vlog_arg, logic_device, logic_ve, logic_vv) + \
                    (' -m "%s"'%ip_vv if ip_vv else "")
        )
        env.Replace(
            PREER=supra_exe,
            PREERFLAGS=[
                "-L",  slash_relpath(logic_log),
                "-X",  "set LOGIC_DEVICE {%s}"%logic_device,
                "-X",  "set LOGIC_DESIGN {%s}"%logic_design,
                "-X",  "set LOGIC_DIR  {%s}"%slash_relpath(logic_dir),
                "-X",  "set LOGIC_VV   {%s}"%slash_relpath(logic_vv, logic_dir),
                "-X",  "set LOGIC_ASF  {%s}"%slash_relpath(logic_asf, logic_dir),
                "-X",  "set LOGIC_PRE  {%s}"%slash_relpath(logic_pre, logic_dir),
                "-X",  "set LOGIC_POST {%s}"%slash_relpath(logic_post, logic_dir),
                "-X",  "set DESIGN_ASF  {%s}"%slash_relpath(design_asf, logic_dir),
                "-X",  "set DESIGN_PRE  {%s}"%slash_relpath(design_pre, logic_dir),
                "-X",  "set DESIGN_POST {%s}"%slash_relpath(design_post, logic_dir),
                "-X",  "set LOGIC_FORCE {%s}"%("true" if logic_force else "false"),
                "-X",  "set VEX_FILE    {%s}"%slash_relpath(logic_ve, logic_dir),
                "-X",  "set AGF_FILE    {%s}"%slash_relpath(design_agf, logic_dir),
                "-X",  "set SDC_FILE    {%s}"%slash_relpath(design_sdc, logic_dir),
                "-X",  "set IP_VV {%s}"%slash_relpath(ip_vv, logic_dir),
                "-X",  "set ORIGINAL_PIN {-}"
            ] + ([] if not logic_compress else ["-X", "set LOGIC_COMPRESS true"]),
            PREERCMD='"$PREER" $PREERFLAGS %s -F "%s"'%(supra_arg, slash_path(pre_logic_tcl))
        )
        vlog_actions += [env.VerboseAction(ensure_logic_clean_dir, " "),
                         env.VerboseAction("$VLOGERCMD",
                                           "Generating LOGIC netlist template %s and header %s from %s"
                                           %(basename(logic_vv), basename(logic_h), basename(logic_ve)))]
        gen_actions += [env.VerboseAction("$PREERCMD",
                                          "Preparing LOGIC compilation project %s"%logic_design)]

    else:
        if need_preip:
            env.Replace(
                IPVLOGER="$PYTHONEXE",
                IPVLOGERFLAGS=[
                    "\"%s\""%vlog_exe, "-c", logic_ip_h
                ],
                IPVLOGERCMD= \
                        '"$IPVLOGER" $IPVLOGERFLAGS %s -s -m "%s" -d "%s" -i "%s" "%s"'%(vlog_arg, logic_ip_vv, logic_device, logic_ip_ve, logic_ve)
            )
            env.Replace(
                # Do not pass board level ASF logic files
                IPPREER=supra_exe,
                IPPREERFLAGS=[
                    "-L",  logic_ip_log,
                    "-X",  "set LOGIC_DEVICE {%s}"%logic_device,
                    "-X",  "set LOGIC_DESIGN {%s}"%logic_ip_design,
                    "-X",  "set LOGIC_MODULE {%s}"%logic_ip_design,
                    "-X",  "set IP_DESIGN {%s}"%logic_ip_design,
                    "-X",  "set IP_INSTALL_DIR {%s}"%slash_relpath(logic_ip_install_dir, logic_dir),
                    "-X",  "set LOGIC_DIR  {%s}"%slash_relpath(logic_ip_dir),
                    "-X",  "set LOGIC_VV   {%s}"%slash_relpath(logic_ip_vv, logic_dir),
                    "-X",  "set LOGIC_ASF  {}",
                    "-X",  "set LOGIC_PRE  {}",
                    "-X",  "set LOGIC_POST {}",
                    "-X",  "set LOGIC_FORCE {%s}"%("true" if logic_force else "false"),
                    "-X",  "set VEX_FILE    {%s}"%slash_relpath(logic_ve, logic_dir),
                    "-X",  "set AGF_FILE    {%s}"%slash_relpath(design_agf, logic_dir),
                    "-X",  "set SDC_FILE    {%s}"%slash_relpath(design_sdc, logic_dir),
                    "-X",  "set ORIGINAL_PIN {-}"
                ] + ([] if not logic_compress else ["-X", "set LOGIC_COMPRESS true"]),
                IPPREERCMD='"$IPPREER" $IPPREERFLAGS %s -F "%s"'%(supra_arg, slash_path(pre_logic_tcl))
            )
            ipvlog_actions += [env.VerboseAction(ensure_logic_ip_clean_dir, " "),
                             env.VerboseAction("$IPVLOGERCMD",
                                               "Generating LOGIC IP netlist template %s from %s"
                                               %(basename(ip_vv), basename(logic_ve)))]
            ipgen_actions += [env.VerboseAction("$IPPREERCMD",
                                              "Preparing LOGIC IP compilation project %s"%logic_ip_design)]

        env.Replace(
           VLOGER="$PYTHONEXE",
           VLOGERFLAGS=[
               "\"%s\""%vlog_exe, "-c", logic_h
           ],
           VLOGERCMD= \
                   '"$VLOGER" $VLOGERFLAGS %s -d "%s" "%s" "%s"'%(vlog_arg, logic_device, logic_ve, logic_vx) + \
                   (' -m "%s"'%ip_vx if ip_vx else "") + \
                   (' -i "%s"'%ip_ve if ip_ve else "")
        )
        env.Replace(
            GENER=supra_exe,
            GENERFLAGS=[
                "-L",  slash_relpath(logic_log),
                "-X",  "set LOGIC_DB {%s}"%slash_relpath(logic_db, logic_dir),
                "-X",  "set LOGIC_DEVICE {%s}"%logic_device,
                "-X",  "set LOGIC_DESIGN {%s}"%logic_design,
                "-X",  "set LOGIC_DIR  {%s}"%slash_relpath(logic_dir),
                "-X",  "set LOGIC_VX   {%s}"%slash_relpath(logic_vx, logic_dir),
                "-X",  "set LOGIC_ASF  {%s}"%slash_relpath(logic_asf, logic_dir),
                "-X",  "set LOGIC_PRE  {%s}"%slash_relpath(logic_pre, logic_dir),
                "-X",  "set LOGIC_POST {%s}"%slash_relpath(logic_post, logic_dir),
                "-X",  "set IP_ASF  {%s}"%slash_relpath(ip_asf, logic_dir),
                "-X",  "set IP_PRE  {%s}"%slash_relpath(ip_pre, logic_dir),
                "-X",  "set IP_POST {%s}"%slash_relpath(ip_post, logic_dir),
                "-X",  "set DESIGN_ASF  {%s}"%slash_relpath(design_asf, logic_dir),
                "-X",  "set DESIGN_PRE  {%s}"%slash_relpath(design_pre, logic_dir),
                "-X",  "set DESIGN_POST {%s}"%slash_relpath(design_post, logic_dir),
                "-X",  "set LOGIC_BIN {%s}"%slash_relpath(logic_bin, logic_dir),
                "-X",  "set IP_SDC {%s}"%slash_relpath(ip_sdc, logic_dir),
            ] + ([] if not logic_compress else ["-X", "set LOGIC_COMPRESS true"]),
            GENERCMD='"$GENER" $GENERFLAGS %s -F "%s"'%(supra_arg, slash_path(gen_logic_tcl))
        )
        vlog_actions += [env.VerboseAction(ensure_logic_clean_dir, " "),
                         env.VerboseAction("$VLOGERCMD",
                                           "Generating LOGIC netlist %s and header %s from %s"
                                           %(basename(logic_vx), basename(logic_h), basename(logic_ve)))]
        gen_actions += [env.VerboseAction("$GENERCMD",
                                          "Compiling LOGIC netlist into bitstream %s"%basename(logic_bin))]

    if need_prelogic:
        target_buildvlog = env.Command([logic_vv, logic_h],
            [abspath(logic_ve)],
            env.VerboseAction(vlog_actions,
                             "Generating LOGIC netlist template %s and header %s from %s"
                             %(basename(logic_vv), basename(logic_h), basename(logic_ve))))
        target_buildgen = env.AddCustomTarget(
            name="prelogic",
            dependencies=abspath(logic_vv),
            actions=gen_actions,
            title="Prepare LOGIC",
            description="Preparing LOGIC compilation project %s"%logic_design,
            always_build=True
        )

    else:
        if need_preip:
            target_buildipvlog = env.Command([logic_ip_ve, logic_ip_h],
                [abspath(logic_ve)],
                env.VerboseAction(ipvlog_actions,
                                 "Generating LOGIC IP netlist template %s from %s"
                                 %(basename(logic_ip_vv), basename(logic_ve))))
            target_buildipgen = env.AddCustomTarget(
                name="preip",
                dependencies=abspath(logic_ip_ve),
                actions=ipgen_actions,
                title="Prepare IP",
                description="Preparing LOGIC IP compilation project %s"%logic_design,
                always_build=True
            )
            env.Alias("buildipvlog", target_buildipvlog)

        target_buildvlog = env.Command([logic_vx, logic_h],
            [abspath(logic_ve)] + \
                ([abspath(ip_vx)] if ip_vx else []) + \
                ([abspath(ip_ve)] if ip_ve else []),
            env.VerboseAction(vlog_actions,
                              "Generating LOGIC netlist %s and header %s from %s"
                              %(basename(logic_vx), basename(logic_h), basename(logic_ve))))

        target_buildgen = env.Command(logic_bin,
            [abspath(logic_vx)] + \
                ([abspath(logic_asf)] if logic_asf else []) + \
                ([abspath(logic_pre)] if logic_pre else []) + \
                ([abspath(logic_post)] if logic_post else []) + \
                ([abspath(ip_asf)] if ip_asf else []) + \
                ([abspath(ip_pre)] if ip_pre else []) + \
                ([abspath(ip_post)] if ip_post else []) + \
                ([abspath(ip_sdc)] if ip_sdc else []) + \
                ([abspath(design_asf)] if design_asf else []) + \
                ([abspath(design_pre)] if design_pre else []) + \
                ([abspath(design_post)] if design_post else []) + \
                ([abspath(design_sdc)] if design_sdc else []),
            env.VerboseAction(gen_actions,
                              "Compiling LOGIC netlist into bitstream %s"%basename(logic_bin)))

    logic_depends = [join(sdk_dir, "src", "system.h")]
    if not logic_skip:
        env.Depends(logic_depends, logic_h)
    env.Alias("buildvlog", target_buildvlog)

#
# Flash lock/unlock/wipe targets
#

def sleep_001(source, target, env):
    wait_001()
def sleep_01(source, target, env):
    wait_01()
def sleep_05(source, target, env):
    wait_05()
def sleep_10(source, target, env):
    wait_10()

lock_actions = []
unlock_actions = []
opterase_actions = []
wipe_actions = []

if protocol_base == "serial":
    env.Replace(
        LOCKER=flasher_exe,
        LOCKERFLAGS=[
            "-m", "8e1", "-b", custom_speed, "-j"
        ],
        LOCKCMD='"$LOCKER" $LOCKERFLAGS %s "${__configure_upload_port(__env__)}"'%(flasher_arg)
    )
    lock_actions += [
        port_action,
        env.VerboseAction("$LOCKCMD", "Lock device")
    ]
    env.Replace(
        UNLOCKER=flasher_exe,
        UNLOCKERFLAGS=[
            "-m", "8e1", "-b", custom_speed, "-k"
        ],
        UNLOCKCMD='"$UNLOCKER" $UNLOCKERFLAGS %s "${__configure_upload_port(__env__)}"'%(flasher_arg)
    )
    unlock_actions += [
        port_action,
        env.VerboseAction("$UNLOCKCMD", "Unlock device")
    ]
    env.Replace(
        OPTERASER=flasher_exe,
        OPTERASERFLAGS=[
            "-m", "8e1",
            "-T", boot_address, "-B", custom_speed, "-X", boot_version,
            "-b", custom_speed, "-O"
        ],
        OPTERASECMD='"$OPTERASER" $OPTERASERFLAGS %s "${__configure_upload_port(__env__)}"'%(flasher_arg)
    )
    opterase_actions += [
        port_action,
        env.VerboseAction("$OPTERASECMD", "Erase option bytes")
    ]
    wipe_actions += unlock_actions + [env.VerboseAction(sleep_01, "sleep .:.\n")] + \
                    opterase_actions + [env.VerboseAction(sleep_01, "sleep .:.\n")] + unlock_actions
else:
    env.Replace(
        LOCKER="$PYTHONEXE",
        LOCKERFLAGS=oo_args+[
            "-p"
        ] + (["-j" , transport_if] if transport_if else []),
        LOCKCMD='$LOCKER $LOCKERFLAGS'
    )
    lock_actions += [
        env.VerboseAction("$LOCKCMD", "Lock device")
    ]
    env.Replace(
        UNLOCKER="$PYTHONEXE",
        UNLOCKERFLAGS=oo_args+[
            "-u"
        ] + (["-j" , transport_if] if transport_if else []),
        UNLOCKCMD='$UNLOCKER $UNLOCKERFLAGS'
    )
    unlock_actions += [
        env.VerboseAction("$UNLOCKCMD", "Unlock device")
    ]
    env.Replace(
        OPTERASER="$PYTHONEXE",
        OPTERASERFLAGS=oo_args+[
            "-o"
        ] + (["-j" , transport_if] if transport_if else []),
        OPTERASECMD='$OPTERASER $OPTERASERFLAGS'
    )
    opterase_actions += [
        env.VerboseAction("$OPTERASECMD", "Erase option bytes")
    ]
    env.Replace(
        WIPER="$PYTHONEXE",
        WIPERFLAGS=oo_args+[
            "-ou"
        ] + (["-j" , transport_if] if transport_if else []),
        WIPECMD='$WIPER $WIPERFLAGS'
    )
    wipe_actions += [
        env.VerboseAction("$WIPECMD", "Wipe device")
    ]

#env.AddCustomTarget(
#    name="lock",
#    dependencies=None,
#    actions=lock_actions,
#    title="Lock Flash",
#    description="Setting Flash read protect (Lock)",
#    always_build=True
#)
env.AddCustomTarget(
    name="unlock",
    dependencies=None,
    actions=unlock_actions,
    title="Unlock Flash",
    description="Setting Flash read unprotect (Unlock)",
    always_build=True
)
env.AddCustomTarget(
    name="wipe",
    dependencies=None,
    actions=wipe_actions,
    title="Wipe Flash",
    description="Wiping Flash to Factory Settings",
    always_build=True
)

#
# Custom targets
#

def garbage_clean(target, source, env):
    for p in psutil.process_iter():
        if platformio_exe0 == p.name() or \
           openocd_exe0 == p.name() or \
           jlink_dbg_exe0 == p.name():
            info("Process kill %s : %s.\n" % (p.pid,p.name))
            p.kill()
env.AddCustomTarget(
    name="gc",
    dependencies=None,
    actions=[env.VerboseAction(garbage_clean, "")],
    title="Garbage Clean",
    description="Cleaning up build leftover garbages",
    always_build=True
)

def logic_clean(target, source, env):
    if is_dir_or_link(project_logic_dir):
        print("Removing %s"%get_relpath(project_logic_dir))
        fs.rmtree(project_logic_dir)
    else:
        print("Logic environment is clean")
    print("Done cleaning")
env.AddCustomTarget(
    name="logic_clean",
    dependencies=None,
    actions=[env.VerboseAction(logic_clean, " ")],
    title="Clean LOGIC",
    description="Cleaning up logic environment",
    always_build=True
)

#
# Downloader
batch_bin = join(project_dir, "%s.bin"%batch)

flash_addr, flash_size = get_flash_info()
batch_actions = []
extra_bins = []
dash_lc = '--logic-config "%s"' % abspath(logic_bin)
dash_la = '--logic-address %s' % logic_address 
dash_i = '-i "%s"'%abspath(program_bin)
dash_a = '-a %s'%(boot_addr or f"0x{flash_addr:x}")
if not user_logic_bin_addr:
    pass
elif not user_logic_bin_addr[0] and not user_logic_bin_addr[1]:
    dash_lc = ""
    dash_la = ""
else:
    if user_logic_bin_addr[0]:
        dash_lc = '--logic-config "%s"' % abspath(user_logic_bin_addr[0])
        extra_bins.append(user_logic_bin_addr[0])
    if user_logic_bin_addr[1]:
        dash_la = '--logic-address %s' % user_logic_bin_addr[1]
for user_bin_addr in user_bin_addrs:
    try:
        user_bin_data = os.path.basename(user_bin_addr[0])
        int(user_bin_data, 0)
        dash_i += ' "%s"'%user_bin_data
    except:
        extra_bins.append(user_bin_addr[0])
        dash_i += ' "%s"'%abspath(user_bin_addr[0])
    dash_a += ' %s'%user_bin_addr[1]
env.Replace(
    BATCHER="$PYTHONEXE",
    BATCHERFLAGS=[
        "\"%s\""%batch_exe
    ] + (["--logic-compress"] if logic_compress else [])
      + (["--logic-encrypt"] if logic_encrypt else [])
      + (["--flash-size", flash_size])
      + (["--lock"] if lock_flash else []),
    BATCHCMD= '"$BATCHER" $BATCHERFLAGS %s %s %s %s %s -o "%s"'
                    %(batch_arg, dash_lc, dash_la, dash_i, dash_a, abspath(batch_bin))
)
batch_actions += [env.VerboseAction("$BATCHCMD",
                                    "Generating BATCH bin %s"%basename(batch_bin))]

batch_upload_actions = []
if protocol_base == "serial":
    env.Replace(
        BATCH_UPLOADER=flasher_exe,
        BATCH_UPLOADERFLAGS= [
            "-m", "8e1", "-G", 1,
            "-T", boot_address, "-L", "-B", custom_speed, "-X", boot_version,
            "-b", upload_speed, "-R",
            "-Z"
        ],
        BATCH_UPLOADCMD='"$BATCH_UPLOADER" $BATCH_UPLOADERFLAGS %s -N "$SOURCE" "${__configure_upload_port(__env__)}"'%(flasher_arg)
    )
    batch_upload_actions += [
        port_action,
        env.VerboseAction("$BATCH_UPLOADCMD",
            "Uploading $SOURCE @${__configure_upload_port(__env__)}")
    ]
else:
    env.Replace(
        BATCH_UPLOADER="$PYTHONEXE",
        BATCH_UPLOADERFLAGS=oo_args+[
        ] + (["-j" , transport_if] if transport_if else []),
        BATCH_UPLOADCMD='$BATCH_UPLOADER $BATCH_UPLOADERFLAGS %s -N "$SOURCE"'%(oo_arg)
    )
    batch_upload_actions += [
        env.VerboseAction("$BATCH_UPLOADCMD", "Uploading $SOURCE to %s"%logic_address)
    ]

env.Command(batch_bin,
            ([target_buildlogic] if not user_logic_bin_addr else [project_ini]) + [target_buildprog] + extra_bins,
            env.VerboseAction(batch_actions,
                    "Generating BATCH bitstream %s"%basename(batch_bin)))
target_buildbatch = env.Alias("buildbatch", abspath(batch_bin))
env.AddCustomTarget(
    name="batch",
    dependencies=batch_bin,
    actions=None,
    title="Create Batch",
    description="Create Batch bitstream %s"%basename(batch_bin),
    always_build=True
)

env.AddCustomTarget(
    name="batch_upload",
    dependencies=batch_bin,
    actions=batch_upload_actions,
    title="Upload Batch",
    description="Upload Batch bitstream %s"%basename(batch_bin),
    always_build=True
)

#
# Setup default targets
#

Default([target_buildprog, target_size])

if 0:
  import pprint
  pprint.pprint(list(env.values()), indent=4)
  exit()
