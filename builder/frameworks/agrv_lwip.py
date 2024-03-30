import os
from common import *

if not env.get("SDK_IMPORTED", False):
    # Force SDK package to build if not specified in framework list
    env.SConscript("agrv_sdk.py")
