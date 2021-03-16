#!/usr/bin/env python3

# Process Creation
import subprocess

# Git (Requires GitPython)
import git

# System
import sys

# Environment Variables, Dir Creation
from os import environ, mkdir, path

'''
    I'm sick and tired of having to do these steps manually, so we are unfucking this RIGHT NOW

    Steps:

    Pre: Make sure everything and it's grandmother is installed so the build doesn't fail
    This means, we need

    python3, python3-pip (pip: gitpython)
    git
    build-essential
    clang

    1. Check env:BuildStubs=1
        a. If so, then check which firmware version the stubs should be generated
        b. Build traphouse (OrbisStubGen)
        c. Generate stubs for the specific version
'''

'''
User Configuration Options
'''
# Git relative path
g_GitPath = "/__git"

# Build relative path
g_BuildPath = "/__build"

# The current branch of ps4libdoc to use (default: 7.00)
g_LibDocVersion = "7.00"

'''
WARNING: Do not touch
Script Configuration Options

These *must* be set by the pre-req's for the specific platform before the script will continue
'''
g_GitDirectory = ""
g_BuildDirectory = ""
g_ToolchainDirectory = ""

def IsInstalled(p_Tool):
    try:
        subprocess.call([p_Tool, "--version"], stdout=subprocess.PIPE)
        return True
    except FileNotFoundError:
        return False

def InstallProgram(p_Tool):
    try:
        subprocess.call(["sudo apt install " + p_Tool + " -y"], stdout=subprocess.PIPE)
        return True
    except:
        return False

def InstallPythonModule(p_Module):
    try:
        subprocess.check_call(["python3", "-m", "pip", "install", p_Module], stdout=subprocess.PIPE)
        return True
    except Exception as e:
        print(e)
        return False

def linux_install_deps():
    s_IsPython3Installed = IsInstalled("python3")
    s_IsGitInstalled = IsInstalled("git")
    s_IsGccInstalled = IsInstalled("gcc")
    s_IsClangInstalled = IsInstalled("clang")

    # If python3 is not installed on the running machine
    if not s_IsPython3Installed:
        # Install python 3
        if not InstallProgram("python3 python3-pip"):
            print("installing python3 failed.")
            return False
    
    # Install gitpython with pip install gitpython
    #if not InstallPythonModule("gitpython"):
    #    print("installing gitpython failed.")
    #    return False

    # If gcc is not installed
    if not s_IsGccInstalled:
        # Install the entire build-essential toolset
        if not InstallProgram("build-essential"):
            print("installing build-essential failed.")
            return False
    
    if not s_IsClangInstalled:
        # Install clang, lldb
        if not InstallProgram("clang lldb"):
            print("installing clang/lldb failed.")
            return False
    
    return True

def win_prereq():
    print("Init Windows Pre-Reqs")
    return True

def linux_prereq():
    global g_ToolchainDirectory

    print("Init Linux Pre-Reqs")

    # Install tooling dependencies
    if not linux_install_deps():
        print("failed to install linux dependencies...")
        return False

    # Check if the environment variable for OO_PS4_TOOLCHAIN exists already
    g_ToolchainDirectory = environ.get("OO_PS4_TOOLCHAIN")
    if g_ToolchainDirectory is None:
        g_ToolchainDirectory = input("Please enter toolchain install path [ex: /opt/oosdk]: ")
        if g_ToolchainDirectory is None:
            print("invalid toolchain path.")
            return False
    
    # Create the toolchain path if it does not exist already
    if not path.exists(g_ToolchainDirectory):
        print("Toolchain Path (" + g_ToolchainDirectory + ") does not exist, creating...")
        try:
            mkdir(g_ToolchainDirectory, 0o777)
        except:
            print("could not create toolchain path.")
            return False
    
    return True

def macos_prereq():
    print("Init MacOS Pre-Reqs")
    return True

def main():
    global g_GitDirectory
    global g_BuildDirectory
    global g_ToolchainDirectory
    # Determine which OS we are running on
    s_Platform = sys.platform
    s_Result = False
    if s_Platform.startswith("linux"):
        s_Result = linux_prereq()
    elif s_Platform.startswith("win32"):
        s_Result = win_prereq()
    elif s_Platform.startswith("darwin"):
        s_Result = macos_prereq()
    else:
        print("no valid os found.")
        return
    
    # Check to see if all dependencies got installed properly
    if not s_Result:
        print("pre-reqs failed to install.")
        return False
    
    # Get the path of the git directory
    g_GitDirectory = g_ToolchainDirectory + g_GitPath
    print("git directory: " + g_GitDirectory)
    if not path.exists(g_GitDirectory):
        print("Creating git direrctory (" + g_GitDirectory + ")")
        try:
            mkdir(path=g_GitDirectory, mode=0o777)
        except:
            print("could not create git directory.")
            return False
    
    # Get the build directory
    g_BuildDirectory = g_ToolchainDirectory + g_BuildPath
    print("build directory: " + g_BuildDirectory)
    if not path.exists(g_BuildDirectory):
        print("Creating build directory (" + g_BuildDirectory + ")")
        try:
            mkdir(path=g_BuildDirectory, mode=0o777)
        except:
            print("could not create build directory.")
            return False
    
    # Clone the ps4libdoc repo
    s_LibDocRepo = git.Repo.clone_from("https://github.com/idc/ps4libdoc.git", g_GitDirectory + "/ps4libdoc", branch=g_LibDocVersion)

    # Clone the toolchain repo
    s_ToolchainRepo = git.Repo.clone_from("https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain.git", g_GitDirectory + "/toolchain", branch="master")

    # Generate le-stubs, this will have to be ran from the cwd of the orbisLibGen script
    subprocess.check_call(["python3", g_GitDirectory + "/toolchain/src/tools/orbisLibGen/generate.py", g_GitDirectory + "/ps4libdoc"], stdout=subprocess.PIPE)

if __name__ == "__main__":
    # Execute only if run as a script
    main()
