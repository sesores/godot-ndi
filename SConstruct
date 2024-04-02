#!/usr/bin/env python
import os
import sys
import subprocess


env = SConscript("godot-cpp/SConstruct")

env.Append(CPPPATH=["src/"])

sources = Glob("src/*.cpp")

if env["platform"] == "macos":
    env['SHLIBPREFIX'] = ''

    env.Append(CPPPATH=["/Library/NDI SDK for Apple/include"])
    env.Append(LIBPATH=["lib/"])
    env.Append(LIBS=["libndi"])
    
    target = "demo/bin/ndi.{}.{}.framework/ndi.{}.{}".format(
        env["platform"], 
        env["target"], 
        env["platform"], 
        env["target"]
    )

    library = env.SharedLibrary(target, source=sources, )

    def sys_exec(args):
        proc = subprocess.Popen(args, stdout=subprocess.PIPE, text=True)
        (out, err) = proc.communicate()
        return out.rstrip("\r\n").lstrip()

    def change_id(self, arg, env, executor = None):
        sys_exec(["install_name_tool", "-id", "@rpath/libndi.dylib", target])
        sys_exec(["install_name_tool", "-change", "@rpath/libndi.dylib", "@loader_path/../libndi.dylib", target])
    
    change_id_action = Action('', change_id)

    AddPostAction(library, change_id_action)

else:
    library = env.SharedLibrary(
        "demo/bin/ndi{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
