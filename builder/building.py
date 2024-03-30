import os, re, glob
import rtconfig

remove_components = []

class RtGroup:
    def __init__(self, name, src, **kwargs):
        self.sconscript = name == "__SConscript__"
        self.src = src
        self.depend = kwargs.get("depend", [])
        self.CPPPATH = kwargs.get("CPPPATH", [])
        self.ASFLAGS = kwargs.get("ASFLAGS", [])
        self.CCFLAGS = kwargs.get("CCFLAGS", [])
        self.LINKFLAGS = kwargs.get("LINKFLAGS", [])
        if type(self.src) != list:
            self.src = [self.src]
        if type(self.depend) != list:
            self.depend = [self.depend]
        if type(self.CPPPATH) != list:
            self.CPPPATH = [self.CPPPATH]
        if type(self.ASFLAGS) != list:
            self.ASFLAGS = [self.ASFLAGS]
        if type(self.CCFLAGS) != list:
            self.CCFLAGS = [self.CCFLAGS]
        if type(self.LINKFLAGS) != list:
            self.LINKFLAGS = [self.LINKFLAGS]

def DefineGroup(name, src, **kwargs):
    global depends
    group = RtGroup(name, src, **kwargs)
    depends += group.depend
    return [group]

def GetDepend(dep):
    global depends
    return any([("*" in depends or dd in depends) for dd in ([dep] if type(dep)==str else dep)])

def GetCurrentDir():
    return os.getcwd()

def Split(val):
    return [x for x in re.split("\s", val) if x]

def Glob(pattern):
    return glob.glob(pattern)

def SrcRemove(src, remove_src):
    if remove_src in src:
        src.remove(remove_src)

def SConscript(script):
    return DefineGroup("__SConscript__", script)

def GetNewLibVersion(rc):
    return rc.VERSION

def Import(script):
    pass

def Return(group):
    pass
