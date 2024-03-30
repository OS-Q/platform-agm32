import os, sys, re

size_tool = sys.argv[1]
elf_file = sys.argv[2]
bin_file = os.path.splitext(elf_file)[0] + ".bin"

cmd = " ".join([size_tool, "-A", "-d", elf_file])
result = os.popen(cmd).read()

pattern = r"^\.flash\s+(\d+)\s+\d+\s*"
regexp = re.compile(pattern)
flash = 0
for line in result.split("\n"):
    line = line.strip()
    if not line:
        continue
    match = regexp.search(line)
    if not match:
        continue
    flash = int(match.groups()[0])

cmd = " ".join([size_tool, "-B", "-d", elf_file])
result = os.popen(cmd).read()

pattern = r"^(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+"
regexp = re.compile(pattern)
sizes = []
for line in result.split("\n"):
    line = line.strip()
    if not line:
        continue
    match = regexp.search(line)
    if not match:
        continue
    sizes = [int(value) for value in match.groups()]

try:
    bin_size = os.path.getsize(bin_file)
except:
    bin_size = 0

print("Flash\tRAM")
try:
    flash_size = sizes[0]+sizes[1]
    ram_size = sizes[1]+sizes[2]-flash
    if (bin_size > flash_size):
        flash_size = bin_size
    print("%s\t%s" % (flash_size, ram_size))
except:
    print("0\t0")

exit(0)
