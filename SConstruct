#!python
import os

opts = Variables([], ARGUMENTS)

env = DefaultEnvironment()

platform_array = ["", "windows", "linuxbsd", "macos", "x11", "linux", "osx"]
opts.Add(EnumVariable("platform", "Compilation platform", "", platform_array))
opts.Add(BoolVariable("use_llvm", "Use the LLVM / Clang compiler", "no"))

opts.Update(env)

if env["platform"] == "macos":
    env["platform"] = "osx"
elif env["platform"] in ("x11", "linuxbsd"):
    env["platform"] = "linux"
elif env["platform"] == "bsd":
    env["platform"] = "freebsd"

if env["platform"] == "":
    print("No valid target platform selected.")
    print("Valid platforms: " + ", ".join(platform_array))
    quit()

platform = env["platform"]

if platform == "osx":
    if not env["use_llvm"]:
        env["use_llvm"] = "yes"
    env.Append(CCFLAGS=["-g", "-O3", "-arch", "x86_64", "-std=c++14"])
    env.Append(LINKFLAGS=["-arch", "x86_64"])
elif platform == "linux":
    env.Append(CCFLAGS=["-fPIC", "-g", "-O3"])
elif platform == "windows":
    env = Environment(ENV=os.environ)
    opts.Update(env)

    env.Append(
        CCFLAGS=[
            "-DWIN32",
            "-D_WIN32",
            "-D_WINDOWS",
            "-W3",
            "-GR",
            "-D_CRT_SECURE_NO_WARNINGS",
            "-O2",
            "-EHsc",
            "-DNDEBUG",
            "-MD",
        ]
    )

if env["use_llvm"] == "yes":
    env["CC"] = "clang"
    env["CXX"] = "clang++"

env.Append(
    CPPPATH=[
        "godot-cpp/include/",
        "godot-cpp/include/core/",
        "godot-cpp/include/gen/",
        "godot-cpp/godot-headers/",
        "FFmpeg/"
    ]
)

libDir = "recorder/bin"

def get_libs():
    libs = [env.File(os.path.join("godot-cpp/bin", "libgodot-cpp.%s.release.64%s" % (platform, env["LIBSUFFIX"])))]
    for f in os.listdir(libDir):
        if f.endswith(env["SHLIBSUFFIX"]):
            libs.append(env.File(libDir + "/" + f))
    return libs

env.Append(LIBS=get_libs())

env.Append(LIBPATH=["godot-cpp/bin/", "recorder/bin/"])

library = env.SharedLibrary("recorder/bin/librecorder{}".format(env["SHLIBSUFFIX"]), source=Glob("src/*.cpp"))
Default(library)
