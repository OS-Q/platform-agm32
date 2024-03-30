import sys, os, re, time, shutil
from platform import system
from platformio import compat, fs
from os import makedirs, getcwd
from os.path import isdir, isfile, islink, join, dirname, basename, relpath, abspath, splitext, getsize
from distutils import util
from click import secho

import SCons
import SCons.Script
from SCons.Script import (ARGUMENTS, COMMAND_LINE_TARGETS, AlwaysBuild,
                          Builder, Default, DefaultEnvironment)
from SCons.Errors import (BuildError, UserError, StopError)

module_dir = dirname(__file__)

#init_start = "0x00010eea"
init_start = ""

is_win = system() == "Windows"
is_verbose = int(ARGUMENTS.get("PIOVERBOSE", 0))
env = DefaultEnvironment()
pio = env.PioPlatform()
board_config = env.BoardConfig()
board = board_config.id
assert board
envname = env.subst("$PIOENV")
frameworks = env.get("PIOFRAMEWORK", [])
mainprog = env.get("PIOMAINPROG")

project_dir = env.subst("$PROJECT_DIR")
project_src_dir = env.subst("$PROJECT_SRC_DIR")
project_inc_dir = env.subst("$PROJECT_INCLUDE_DIR")
project_workspace_dir = env.subst("$PROJECT_WORKSPACE_DIR")
project_build_dir = env.subst("$PROJECT_BUILD_DIR")
project_env_dir   = join(project_build_dir, envname)
project_logic_dir = join(project_workspace_dir, "logic")
project_ini = join(project_dir, "platformio.ini")

SRC_FILTER_DEFAULT = ["+<*>", "-<.git%s>" % os.sep, "-<.svn%s>" % os.sep]
if env.get("SRC_FILTER", None) is None:
    env.Replace(SRC_FILTER=["+<*> -<.git%s> -<.svn%s>"%(os.sep, os.sep)])

def wait_001():
    time.sleep(0.01)
def wait_01():
    time.sleep(0.1)
def wait_05():
    time.sleep(0.5)
def wait_10():
    time.sleep(1.0)

def check_relpath(path, start=None):
    try:
        relpath(path, start)
    except ValueError:
        return False
    return True

def get_children(nodes):
    children = []
    for node in nodes:
        children.append(node)
        children += get_children(node.all_children())
    return children

def fatal(msg):
    error(msg)
    env.Exit(1)

def error(msg):
    sys.stdout.flush()
    sys.stderr.flush()
    wait_001()
    secho("Error: %s"%msg, nl=False, err=True, fg="red")
    sys.stdout.flush()
    sys.stderr.flush()

def warn(msg):
    sys.stdout.flush()
    sys.stderr.flush()
    wait_001()
    secho("Warn: %s"%msg, nl=False, err=True, fg="yellow")
    sys.stdout.flush()
    sys.stderr.flush()

def info(msg):
    sys.stdout.flush()
    sys.stderr.flush()
    wait_001()
    #secho("Info: %s"%msg, nl=False, err=True, fg="green")
    secho("Info: %s"%msg, nl=False, err=True, fg="green")
    sys.stdout.flush()
    sys.stderr.flush()

def message(msg):
    sys.stdout.flush()
    wait_001()
    secho("%s"%msg, nl=False, err=False)
    sys.stdout.flush()

def verbose(msg):
    if is_verbose:
        message(msg)

def convert_to_bool(val):
    return bool(util.strtobool(val)) if type(val)==str else val

def convert_to_list(val):
    return val if type(val)==list else [val]

def string_to_list(val):
    if type(val) == str:
        return [i for i in re.split("[\s\n,]+", val) if i]
    if type(val) == list and len(val) == 1:
        return string_to_list(val[0])
    return val

def is_file_or_link(file, check_empty=False):
    if isfile(file) or islink(file):
        return getsize(file) > 0 if check_empty else True
    return False

def is_dir_or_link(file):
    return isdir(file) or islink(file)

def slash_path(path_name):
    if not path_name:
        return ""
    return path_name.replace("\\", "/")

def slash_file(path_name):
    if not path_name:
        return ""
    return slash_path(join(".", basename(path_name)))

def get_required_package_dir(package):
    package_dir = pio.get_package_dir(package) or ""
    assert package_dir and isdir(package_dir), ("Missed %s package"%package)
    return package_dir

def get_relpath(path_name, start=None):
    if start is None:
        start = os.getcwd()
    if compat.IS_WINDOWS:
       if path_name.lower().startswith(join(start, "").lower()) or \
          path_name.lower().startswith(join(dirname(start), "").lower()):
          return relpath(path_name, start)
    else:
       if path_name.startswith(join(start, "")) or \
          path_name.startswith(join(dirname(start), "")):
          return relpath(path_name, start)
    return path_name

def slash_relpath(path_name, start=None):
    return slash_path(get_relpath(path_name, start))

def get_sys_env(env_name, default=None):
    return os.environ.get(env_name, default)

def get_project_option(opt_keys, default_val=""):
    keys = convert_to_list(opt_keys)
    for key in keys:
        if key in env.GetProjectOptions(True):
            return env.GetProjectOption(key, default_val)
    return default_val

def get_project_options(opt_keys, default_val=""):
    return string_to_list(get_project_option(opt_keys, default_val))

def get_board_config(config_keys, default_val=""):
    keys = convert_to_list(config_keys)
    for key in keys:
        if key in board_config:
            default_val = board_config.get(key, default_val)
    return get_project_option(keys[1:], default_val)

def get_board_configs(config_keys, default_val=""):
    return string_to_list(get_board_config(config_keys, default_val))

def get_board_dir(board):
    board_json = "%s.json"%board
    boards_dir = get_project_option("boards_dir", "boards")
    if is_file_or_link(join(boards_dir, board_json)):
        if is_dir_or_link(join(boards_dir, board)):
            return abspath(join(boards_dir, board))
        else:
            return abspath(boards_dir)
    boards_dir = join(pio.get_dir(), "boards")
    if is_file_or_link(join(boards_dir, board_json)):
        if is_dir_or_link(join(boards_dir, board)):
            return join(boards_dir, board)
        else:
            return abspath(boards_dir)
    return ""

def get_logic_dir():
    logic_dir = get_project_option("logic_dir", "")
    logic_force = False
    logic_skip = False
    if not logic_dir:
        return [logic_dir, logic_force, logic_skip]
    if logic_dir[-1] == "-":
        logic_dir = logic_dir[:-1]
        logic_skip = True
    elif logic_dir[-1] == "+":
        logic_dir = logic_dir[:-1]
        logic_force = True
    return [abspath(logic_dir), logic_force, logic_skip]

def get_logic_ip(logic_dir, ips):
    ip = ips[0] if ips else ""
    if not logic_dir or not ip:
        return ["", ""]

    logic_ip_str = get_project_option("logic_ip", "False")
    is_logic_ip = False
    ips_dir = logic_dir
    try:
        is_logic_ip = convert_to_bool(logic_ip_str)
    except ValueError:
        is_logic_ip = True
        ips_dir = logic_ip_str
    if not is_logic_ip:
        return ["", ""]

    if not logic_dir or not ip:
        fatal("logic_ip has to be specified with logic_dir and ip_name.\n")
    logic_ip_install_dir = join(ips_dir, ip)
    return [abspath(logic_ip_install_dir), ip]

def get_ip_dir(ip):
    ip_vx = "%s.vx"%ip
    if logic_ip_install_dir:
        ip_dir = logic_ip_install_dir
        if is_file_or_link(join(ip_dir, ip_vx)):
            return ip_dir
    ips_dirs = get_project_options("ips_dir", "ips")
    for ips_dir in ips_dirs:
        ip_dir = join(ips_dir, ip)
        if is_file_or_link(join(ip_dir, ip_vx)):
            return abspath(ip_dir)
    ip_dir = join(get_required_package_dir("framework-agrv_ips"), "ips", ip)
    if is_file_or_link(join(ip_dir, ip_vx)):
        return ip_dir
    return ""

def get_int_value(val):
    if type(val) is int:
        return val
    if type(val) is str:
        if val.startswith("0x") or val.startswith("0X"):
            return int(val, base=16)
        else:
            return int(val)
    return val

def get_device_info():
    device_id = get_int_value(get_board_config("device_id", 0))
    device_flash = get_int_value(get_board_config("upload.maximum_size", 0))
    return "0X%04x:%dKB"%(device_id, int(device_flash/1024)), device_id 

def get_logic_info(logic_address):
    logic_addr = get_int_value(logic_address)
    return "0X%08x"%logic_addr

def get_rom_info():
    rom_addr = get_int_value(get_board_config("upload.rom_address", "0x10000"))
    rom_size = get_int_value(get_board_config("upload.maximum_rom_size", 8192))
    return rom_addr, rom_size

def get_flash_info():
    flash_addr = get_int_value(get_board_config("upload.flash_address", "0x80000000"))
    flash_size = get_int_value(get_board_config("upload.maximum_size", 262144))
    return flash_addr, flash_size

def get_sram_info():
    ram_addr = get_int_value(get_board_config("upload.ram_address", "0x20000000"))
    ram_size = get_int_value(get_board_config("upload.maximum_ram_size", 131072))
    return ram_addr, ram_size

def get_itim_info():
    itim_addr = get_int_value(get_board_config("upload.itim_address", "0x10000000"))
    itim_size = get_int_value(get_board_config("upload.maximum_itim_size", 0x3000))
    return itim_addr, itim_size

def get_logic_file(vext, fext):
    file = get_board_config(["logic.%s"%vext, "logic_%s"%vext], "")
    file = join(project_dir, file) if file else join(board_dir, "board.%s"%fext)
    return file if is_file_or_link(file) else ""

def get_design_file(vext, fext, design):
    file = get_project_option(["design.%s"%vext, "design_%s"%vext], "")
    if not file:
        return ""
    file = join(project_dir, file)
    return file if is_file_or_link(file) else ""

def check_memory_section(address):
    addr = get_int_value(address)
    flash_addr, flash_size = get_flash_info()
    ram_addr, ram_size = get_sram_info()
    itim_addr, itim_size = get_itim_info()
    if (addr >= flash_addr and addr < (flash_addr+flash_size)):
        return "flash"
    if (addr >= ram_addr and addr < (ram_addr+ram_size)):
        return "ram"
    if (addr >= itim_addr and addr < (itim_addr+itim_size)):
        return "itim"
    return "other"

def get_boot_info():
    boot_address = get_board_config("upload.boot_address", "0x20010000")
    return boot_address

def get_upload_info(logic_compress, full_logic_reserve=100, compress_logic_reserve=48):
    flash_addr, flash_size = get_flash_info()
    upload_address = env.get("UPLOAD_ADDRESS", "0x%x" % flash_addr)
    offset_address = get_board_config("upload.offset_address", "")
    upload_start = get_board_config("upload.start", init_start)
    logic_reserve = compress_logic_reserve if logic_compress else full_logic_reserve
    default_logic_address = "0x%x"%(flash_addr+flash_size-1024*logic_reserve)
    logic_address = get_board_config("upload.logic_address", default_logic_address)
    return upload_address, offset_address, upload_start, logic_address

def get_package_build_options(package, option):
    options = get_project_options("build_%s_%s" % (package, option), [])
    if not options and option == "filter":
        options = get_project_options("%s_%s" % (package, option), [])
    return options

def get_build_ram_size():
    return get_board_config("build.maximum_ram_size", "")

def get_build_stack_size():
    return get_board_config("build.stack_size", "")

boot_addr = get_board_config("build.boot_addr", "")
boot_mode = get_board_config("build.boot_mode", "flash").lower()

user_logic_bin_addr = []
user_logic_bin_addr0 = get_project_option("batch_user_logic", None)
if user_logic_bin_addr0 is not None:
    user_logic_bin_addr = user_logic_bin_addr0.split(":")
    if (len(user_logic_bin_addr) > 2):
        fatal("Wrong user_bin format: %s.\n" % user_logic_bin_addr0)
    else:
        for nn in range(2-len(user_logic_bin_addr)):
            user_logic_bin_addr.append("")
        if user_logic_bin_addr[0]:
            user_logic_bin_addr[0] = join(project_dir, user_logic_bin_addr[0])

user_bin_addrs = []
user_bin_addrs0 = get_project_options("batch_user_bin", [])
for user_bin_addr0 in user_bin_addrs0:
    user_bin_addr = user_bin_addr0.split(":")
    if (len(user_bin_addr) != 2):
        fatal("Wrong user_bin format: %s.\n" % user_bin_addr0)
    else:
        if user_bin_addr[0]:
            user_bin_addr[0] = join(project_dir, user_bin_addr[0])
        user_bin_addrs.append(user_bin_addr)

inline_frameworks = get_project_options("inline_framework", [])

tinyusb_param = get_project_options("tinyusb_param", "")
tinyusb_lwip = False
if "agrv_tinyusb" in frameworks and any(s.lower() == "lwip" for s in tinyusb_param):
    tinyusb_lwip = True
    if "agrv_lwip" not in frameworks:
        frameworks.append("agrv_lwip")

lwip_param = get_project_options("lwip_param", "")
lwip_freertos = ""
if "agrv_lwip" in frameworks and any(s.lower() == "freertos" for s in lwip_param):
    lwip_dir = get_required_package_dir("framework-agrv_lwip")
    lwip_freertos = join(lwip_dir, "contrib", "ports", "freertos")

freertos_param = get_project_options("freertos_param", "")
freertos_heap = "heap_4"
if "agrv_freertos" in frameworks:
    for s in freertos_param:
        if s.lower() in ("heap_1", "heap_2", "heap_3", "heap_4", "heap_5"):
            freertos_heap = s.lower()

rtthread_param = get_project_options("rtthread_param", "")
rtthread_depends = get_project_options("rtthread_depend", [])
rtthread_components = []
if "agrv_rtthread" in frameworks:
    for s in get_project_options("rtthread_component", ""):
        rtthread_components.append(s.replace("\\", "/"))

ucos_param = get_project_options("ucos_param", "")
embos_param = get_project_options("embos_param", "")


board_dir = get_board_dir(board)
default_device = "AGRV2KL100"
default_design = board
lock_flash = convert_to_bool(get_project_option(["lock_flash"], "false"))
logic_ve = get_board_config(["logic.ve", "logic_ve"], "")
if logic_ve:
    default_design = splitext(basename(logic_ve))[0]
logic_ve = join(project_dir, logic_ve) if logic_ve else join(board_dir, "board.ve")
logic_device = get_board_config(["logic.device", "logic_device"], default_device)
logic_design = get_board_config(["logic.design", "logic_design"], default_design)
logic_compress = convert_to_bool(get_board_config(["logic.compress", "logic_compress"], "false"))
logic_encrypt = convert_to_bool(get_board_config(["logic.encrypt", "logic_ecnrypt"], "false"))
if logic_encrypt:
    logic_compress = True
logic_asf = get_logic_file("asf", "asf")
logic_pre = get_logic_file("pre_asf", "pre.asf")
logic_post = get_logic_file("post_asf", "post.asf")
design_asf = get_design_file("asf", "asf", logic_design)
design_pre = get_design_file("pre_asf", "pre.asf", logic_design)
design_post = get_design_file("post_asf", "post.asf", logic_design)
design_sdc = get_design_file("sdc", "sdc", logic_design)
design_agf = get_design_file("agf", "agf", logic_design)
ips = get_project_options(["ip_name", "ips"], "")

check_device = convert_to_bool(get_project_option("check_device", True))
check_logic  = get_int_value(get_project_option("check_logic", 1))
if logic_encrypt:
    check_logic = min(check_logic, 2)
check_device = convert_to_bool(get_sys_env("AGM_PIO_CHECK_DEVICE", check_device))
check_logic  = get_int_value(get_sys_env("AGM_PIO_CHECK_LOGIC",  check_logic ))

# By default all env share the same logic dir. Can use board_logic.dir to create it's own sub dir for any env.
board_logic_dir = get_board_config("logic.dir", "")
if board_logic_dir:
    project_logic_dir = join(project_logic_dir, board_logic_dir)
logic_dir, logic_force, logic_skip = get_logic_dir()
logic_ip_install_dir, logic_ip_design = get_logic_ip(logic_dir, ips)
need_preip    = bool(logic_ip_install_dir)
need_prelogic = not need_preip and bool(logic_dir)

if logic_skip:
    logic_bin= join(logic_dir, "%s.bin"%logic_design)
elif need_prelogic:
    logic_vv = join(logic_dir, "%s.v"%logic_design)
    logic_h  = join(logic_dir, "%s.hx"%logic_design)
    logic_bin= join(logic_dir, "%s.bin"%logic_design)
    logic_log= join(logic_dir, "logic_log.txt")
else:
    if need_preip:
        logic_ip_dir = logic_dir
        logic_ip_vv  = join(logic_ip_dir, "%s.v"%logic_ip_design)
        logic_ip_h   = join(logic_ip_dir, "%s.hx"%logic_ip_design)
        logic_ip_log = join(logic_ip_dir, "logic_ip_log.txt")
    logic_dir = project_logic_dir
    logic_vx = join(logic_dir, "%s.vx"%logic_design)
    logic_bin= join(logic_dir, "%s.bin"%logic_design)
    logic_h  = join(logic_dir, "%s.hx"%logic_design)
    logic_db = join(logic_dir, "logic_db")
    logic_log= join(logic_dir, "logic_log.txt")

sys.path.append(join(get_required_package_dir("framework-agrv_sdk"), "etc"))
from building import *
