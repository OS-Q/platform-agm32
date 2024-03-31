import shutil, copy, os, re, platform
import subprocess, sys
from os.path import isdir, join, dirname, basename

from platformio.compat import IS_WINDOWS
from platformio.device.list.util import list_serial_ports
from platform import system
from platformio.managers.platform import PlatformBase
from platformio.util import get_systype
from platformio.project.options import ProjectOptions
from platformio.project.config import ProjectConfig
from platformio import app, __version__
from click import secho

#import locale
#locale.setlocale(locale.LC_ALL, 'en_US.UTF-8')
#locale.setlocale(locale.LC_ALL, 'zh_CN.UTF-8')

is_win = system() == "Windows"

def platformio_preamble():
    # Fix Windows path starting with lower case issue when invoked for debug in vscode
    if is_win:
        try:
            cwd = os.getcwd()
            Cwd = cwd[0:1].upper() + cwd[1:]
            os.chdir(join(Cwd, "."))
        except:
            skip
    print("Project diretory: %s" % os.getcwd())
    
    pio_version = __version__
    agrv_version = open(join(dirname(__file__), "VERSION"), 'r', encoding='UTF-8').read()
    print("PlatformIO Core version: %s" % pio_version)
    print("Agm32 Core version: %s" % agrv_version)
    
    def ensure_core_version():
        XVERSION = ["5.2.2", "5.2.3", "5.2.4"]
        YVERSION = "5.2.5"
        if pio_version in XVERSION:
            print("PlatformIO Core version >%s required, but got %s, re-install ..." % (YVERSION, pio_version))
            subprocess.call([sys.executable, "-m", "pip", "install", "-U", "platformio"])
        INTERVAL = 3650
        interval = app.get_setting("check_platformio_interval")
        if interval != INTERVAL:
            interval = INTERVAL
            app.set_setting("check_platformio_interval", interval)
            print("Check Platformio Interval: %s" % interval)
    #ensure_core_version()

#if basename(sys.argv[0]) in ("pio", "platformio", "pio.exe", "platformio.exe"):
#    platformio_preamble()

if ("AGRV_PLATFORMIO_PREAMBLED" not in os.environ):
  if basename(sys.argv[0]) in ("pio", "platformio", "pio.exe", "platformio.exe"):
    platformio_preamble()
    os.environ["AGRV_PLATFORMIO_PREAMBLED"] = "yes"

jlink_dbg_exe = "JLinkGDBServerCL.exe" if is_win else "JLinkGDBServer"
#openocd_gdb_exe = "openocd-gdb"
openocd_gdb_exe = "openocd"

def convert_to_list(val):
    return val if type(val)==list else [val]

def string_to_list(val):
    if type(val) == str:
        return [i for i in re.split("[\s\n,]+", val) if i]
    return val

def append_uniq(to_list, from_list):
    for from_item in convert_to_list(from_list):
        if from_item not in to_list:
            to_list.append(from_item)

class Agm32Platform(PlatformBase):

    def __init__(self, manifest_path):
        super(Agm32Platform, self).__init__(manifest_path)

        # Set the default debug_init_break to empty so that debug is not auto resumed in gdb.py
        ProjectOptions["env.debug_init_break"].default = ""
       #ProjectOptions["env.debug_build_flags"].default = ""
        ProjectOptions["env.debug_build_flags"].default = "-g3 -ggdb3 -gdwarf-4"

        self._debug_tools = []
        self._debug_defaults = []
        self._upload_protocols = []
        for env in self.config.envs():
            env_name = "env:" + env
            env_vals = self.config.items(env=env_name, as_dict=True)
            append_uniq(self._debug_tools, string_to_list(env_vals.get("debug_tools", [])))
            append_uniq(self._debug_defaults, string_to_list(env_vals.get("debug_defaults", [])))
            append_uniq(self._upload_protocols, string_to_list(env_vals.get("upload_protocols", [])))
        append_uniq(self._upload_protocols, self._debug_tools)

    def get_required_package_dir(self, package):
        package_dir = self.get_package_dir(package) or ""
        assert package_dir, ("Missed %s package"%package)
        return package_dir
    
    def configure_default_packages(self, variables, targets):
        return PlatformBase.configure_default_packages(self, variables, targets)

    def get_boards(self, id_=None):
        result = PlatformBase.get_boards(self, id_)
        if not result:
            return result
        if id_:
            return self._add_default_board_info(result)
        else:
            for key, value in result.items():
                result[key] = self._add_default_board_info(result[key])
        return result

    def _add_default_board_info(self, board):
        frameworks = convert_to_list(board.manifest.get("frameworks", []))
        if "agrv_ips" not in frameworks:
            frameworks.append("agrv_ips")
            board.manifest["frameworks"] = frameworks

        debug = board.manifest.get("debug", {})
        upload = board.manifest.get("upload", {})
        debug_tools = convert_to_list(debug.get("onboard_tools", []))
        append_uniq(debug_tools, self._debug_tools)
        debug_defaults = convert_to_list(debug.get("default_tools", []))
        append_uniq(debug_defaults, self._debug_defaults)
        upload_protocols = convert_to_list(upload.get("protocols", []))
        append_uniq(upload_protocols, self._upload_protocols)
        build_mcu = board.manifest.get("build", {}).get("mcu", [])

        sdk_dir = self.get_required_package_dir("framework-agrv_sdk")
        if "tools" not in debug:
            debug["tools"] = {}
        for tool in debug_tools:
            if (tool not in upload_protocols or tool in debug["tools"]):
                continue

            jlink_tool = "-openocd" not in tool
            debug_tool = os.environ.get("AGM_PIO_UPLOAD_PROTOCOL", tool)
            protocol_base = re.sub("-openocd|-swd|-jtag|-dapdirect_swd", "", debug_tool.lower())
            transport_if = re.sub(f"{protocol_base}|-openocd", "", debug_tool.lower()).lstrip("-")

            if tool.lower().startswith("jlink") and jlink_tool:
                assert debug.get("jlink_device"), (
                    "Missed J-Link Device ID for %s" % board.id)

                debug["tools"][tool] = {
                    "server": {
                        "package": "tool-agrv_jlink",
                        "arguments": [
                            "-JLinkDevicesXMLPath", join(sdk_dir, "etc", "jlink", ""), 
                            "-singlerun",
                            "-if", transport_if,
                            "-select", "USB",
                            "-jtagconf", "-1,-1",
                            "-device", debug.get("jlink_device"),
                            "-port", "2331",
                            "-NoGui"
                        ],
                        "executable": jlink_dbg_exe
                    },
                    "onboard": tool in debug_tools,
                    "default": tool in debug_defaults,
                }

            else:
                openocd_dir = self.get_required_package_dir("tool-agrv_openocd")
                cfg_dir = join(sdk_dir or "", "etc")
                server_args = [
                    "-c", "variable ADAPTER %s" % protocol_base,
                    "-c", "variable TRANSPORT %s" % transport_if,
                    "-c", "telnet_port disabled", # Not needed for either gdb pipe or port, and may cause socket error on Windows
                    "-s", cfg_dir,
                    "-s", join("$PACKAGE_DIR","share","openocd","scripts")
                ]

                server_cfg = join(cfg_dir, "%s.cfg"%(build_mcu or "openocd"))
                if os.path.isfile(server_cfg):
                    server_args.extend(["-f", server_cfg])
                else:
                    assert "Unknown debug configuration", board.id

                server_args.extend(["-c", "rtt_start"])
                server_args.extend(["-c", "max_error 10"])
                server_args = [
                    f.replace("$PACKAGE_DIR", openocd_dir) for f in server_args
                ]

                init_cmds = [
                    "set script-extension soft",
                    "set breakpoint pending off",
                    "define pio_reset_halt_target",
                    "  monitor reset halt",
                    "end",
                    "define pio_reset_run_target",
                    "  monitor reset run",
                    "end",
                    "define pio_restart_target",
                    "  pio_reset_halt_target",
                    "  set $DO_BREAK = \"$INIT_BREAK\"",
                    "  if $DO_BREAK[0] == 0",
                    "    thbreak main",
                    "  else",
                    "    $INIT_BREAK",
                    "  end",
                    "  continue",
                    "end",
                    "target extended-remote $DEBUG_PORT",
                    "$LOAD_CMDS",
                    "set $DO_LOAD = \"$LOAD_CMDS\"",
                    "if $DO_LOAD[0] == 0",
                    "  monitor halt",
                    "else",
                    "  pio_restart_target",
                    "end",
                ]

                debug["tools"][tool] = {
                    "port": "127.0.0.1:3333", # Much faster than pipe on Windows.
                    "init_break": "",
                    "onboard": tool in debug_tools,
                    "default": tool in debug_defaults,
                    "init_cmds": init_cmds
                }
                
                if tool.lower().startswith("serial"):
                    port = ""
                    # Auto find the port with "GDB Server" in description, or inteface 3 in location. The location is 
                    # checked because on Windows, description for a CDC device in a USB composite device does not 
                    # include the inteface string. This port can be overwritten by debug_port in .ini.
                    for item in list_serial_ports(as_objects=True):
                        if (item.description and ("GDB Server" in item.description)) or (item.location and (item.location[-2:] == ".3")):
                            port = item.device
                            if IS_WINDOWS and port.startswith("COM") and len(port) > 4:
                                port = "\\\\.\\%s" % port
                    debug["tools"][tool]["port"] = port
                else:
                    debug["tools"][tool]["server"] = {
                        "package": "tool-agrv_openocd",
                        "executable": join("bin", openocd_gdb_exe),
                        "arguments": server_args
                    }

        board.manifest["debug"] = debug

        upload["protocols"] = upload_protocols
        board.manifest["upload"] = upload

        return board

    def configure_debug_session(self, debug_config):
        debug_speed = debug_config.speed or "10000"
        try:
            int(debug_speed)
        except:
            secho("Error: Unrecoginzed debug_speed %s, must be a integer value.\n"%debug_speed, nl=False, err=True, fg="red")
            exit(1)
        if debug_config.tool_name.startswith("serial"):
            if debug_config.speed:
                debug_config.tool_settings["init_cmds"].extend(["monitor adapter speed %s" % debug_speed])
            return
        server_executable = debug_config.tool_settings["server"]["executable"].lower()
        server_arguments = debug_config.tool_settings["server"]["arguments"]
        if debug_speed:
            if "openocd" in server_executable:
                server_arguments = ["-c", "variable ADAPTER_SPEED %s"%debug_speed] + server_arguments 
            elif "jlink" in server_executable:
                server_arguments = server_arguments + ["-speed", debug_speed]
            debug_config.tool_settings["server"]["arguments"] = server_arguments
