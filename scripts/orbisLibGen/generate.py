#!/usr/bin/python3

'''
    Updated orbisLibGen.py

    Orignially Created By: CrazyVoid

    Updated by: SpecterDev
    Rewritten by: kd_tech_

    Febuary 21, 2021
'''
# Argument parser
import argparse
import glob
import os
import json
import codecs
import re

g_HeaderTemplate = ""
g_SourceTemplate = ""

# List of json files to ignore when generating stubs.
# These are useless for our sdk, it will make stuff 
# more clean during the final release of OpenOrbis SDK
g_IgnoredLibraries = ["custom_video_core.elf.json", 
				"I18N.CJK.dll.sprx.json", 
				"I18N.dll.sprx.json", 
				"I18N.MidEast.dll.sprx.json", 
				"I18N.Other.dll.sprx.json", 
				"I18N.Rare.dll.sprx.json", 
				"I18N.West.dll.sprx.json", 
				"JSC.Net.dll.sprx.json", 
				"libSceNKWeb.sprx.json", 
				"libSceNKWebCdlgInjectedBundle.sprx.json", 
				"libSceNKWebKit.sprx.json", 
				"libSceNKWebKitRequirements.sprx.json", 
				"libSceShellUIUtil.sprx.json", 
				"libSceWebKit2.sprx.json", 
				"libSceWebKit2ForVideoService.sprx.json", 
				"libSceWebKit2Secure.sprx.json", 
				"libswctrl.sprx.json", 
				"libswreset.sprx.json", 
				"LoginMgrUIProcess.self.json",
				"LoginMgrWebProcess.self.json",
				"NKNetworkProcess.self.json",
				"NKUIProcess.self.json",
				"NKWebProcess.self.json",
				"NKWebProcessHeapLimited.self.json",
				"orbis-jsc-compiler.self.json",
				"ReactNative.Components.Vsh.dll.sprx.json",
				"ReactNative.Debug.DevSupport.dll.sprx.json",
				"ReactNative.Modules.Vsh.dll.sprx.json",
				"ReactNative.Modules.Vsh.Gct.Telemetry.dll.sprx.json",
				"ReactNative.Modules.Vsh.Gct.Telemetry2.dll.sprx.json",
				"ReactNative.PUI.dll.sprx.json",
				"ReactNative.Vsh.Common.dll.sprx.json",
				"Sce.Facebook.CSSLayout.dll.sprx.json",
				"ScePlayReady.self.json",
				"ScePlayReady2.self.json",
				"SecureUIProcess.self.json",
				"SecureWebProcess.self.json",
				"swagner.self.json",
				"swreset.self.json",
				"UIProcess.self.json",
				"ulobjmgr.sprx.json",
				"webapp.self.json",
				"WebBrowserUIProcess.self.json",
				"WebProcess.self.json",
				"websocket-sharp.dll.sprx.json"]

def camel_case_split(str): 
    return re.findall(r'[A-Z](?:[a-z]+|[A-Z]*(?=[A-Z]|$))', str) 

def generate_assembly(library_name, library_symbols, output_dir):
    global g_HeaderTemplate
    global g_SourceTemplate

    print("generating assembly " + library_name + " ...")

    # This changes SceVshVideoEditWrapper to VshVideoEditWrapper
    lib_name = ""
    if library_name.startswith("Sce") or library_name.startswith("sce"):
        lib_name = library_name[3:]
    elif library_name.startswith("libSce"):
        lib_name = library_name[6:]
    else:
        lib_name = library_name

    # Get the header guard string
    camel_split = camel_case_split(lib_name)
    lib_header_guard = ("__" + "_".join(camel_split) + "_H__").upper()

    lib_header_filename = lib_name + ".h"
    lib_source_filename = lib_name + ".c"

    # Changes the include to be SceBlah.h NOTE: should this be libBlah.h??
    lib_source_content = g_SourceTemplate.replace("%%header_string%%", lib_name + ".h")

    lib_prototypes = [ ]
    lib_functions = [ ]
    for lib_symbol in library_symbols:
        lib_symbol_name = lib_symbol["name"]
        if lib_symbol_name is None:
            continue
        
        print("Function: " + lib_symbol_name)
        lib_prototypes.append("void " + lib_symbol_name + "();\n")
        lib_functions.append("void " + lib_symbol_name + "() { for (;;) ; }\n")
    
    lib_source_content = lib_source_content.replace("%%FUNCTION_LIST%%", "".join(lib_functions))
    lib_header_content = g_HeaderTemplate.replace("%%PROTOTYPE_LIST%%", "".join(lib_prototypes))
    lib_header_content = lib_header_content.replace("%%HEADER_GUARD%%", lib_header_guard)

    if len(lib_prototypes) > 0:
        header_fh = open(output_dir + "/" + lib_header_filename, "w")
        header_fh.write("".join(lib_header_content))
        header_fh.close()

        source_fh = open(output_dir + "/" + lib_source_filename, "w")
        source_fh.write("".join(lib_source_content))
        source_fh.close()

def generate_makefile(output_dir, compiler="clang"):
    print("generating makefile")

    program_list = ""
    rule_list = ""

    o_path = output_dir + "/_lib"
    if not os.path.exists(o_path):
        os.mkdir(o_path, mode=0o777)
    

    for found_path in glob.glob(output_dir + "/*.c"):
        # Get the file name without the extension
        file_name = os.path.basename(found_path)[:-2]

        program_list += file_name + " "

        output_module = o_path + "/" + file_name + ".o"
        output_shared = o_path + "/" + file_name + ".so"

        # Add rule for program
        rule_list += file_name + ":\n"
        rule_list += "\t" + compiler + " -ffreestanding -nostdlib -fno-builtin -fPIC -c " + file_name + ".c -o " + output_module + "\n"
        rule_list += "\t" + compiler + "-shared -ffreestanding -nostdlib -fno-builtin " + output_module + " -o " + output_shared + "\n\n\n"
    
    makefile_content = "all: " + program_list + "\n\n"
    makefile_content += rule_list

    makefile_fh = open(output_dir + "/Makefile", "w")
    makefile_fh.write(makefile_content)
    makefile_fh.close()

def main():
    global g_IgnoredLibraries
    global g_HeaderTemplate
    global g_SourceTemplate

    print('\nPS4 Header and Stub Source Generator\nBy CrazyVoid, kiwidog, SpecterDev\n')

    parser = argparse.ArgumentParser(description="Orbis Header and Stub Generator")
    parser.add_argument("doc_path")
    parser.add_argument("data_path", default="./data")
    parser.add_argument("output_path", default="./build")

    args = parser.parse_args()

    if not os.path.exists(args.doc_path):
        print("invalid ps4libdoc path...")
        return
    
    # check that our data path is valid
    if not os.path.exists(args.data_path):
        print("invalid data path...")
        return
    
    # if the output directory does not exist create it
    if not os.path.exists(args.output_path):
        print("creating directory: " + args.output_path)
        os.mkdir(args.output_path, mode=0o777)
    
    # Read the header template file
    header_template_fh = open(args.data_path + "/header.template", "r")
    g_HeaderTemplate = header_template_fh.read()
    header_template_fh.close()

    # Read the source template file
    source_template_fh = open(args.data_path + "/source.template", "r")
    g_SourceTemplate = source_template_fh.read()
    source_template_fh.close()

    json_list = [ ]

    # Iterate all doc files
    for found_path in glob.glob(args.doc_path + "/**/*.sprx.json", recursive=True):
        found_filename = os.path.basename(found_path)
        if found_filename not in g_IgnoredLibraries:
            input_sprx_content = json.load(codecs.open(found_path, 'r', 'utf-8-sig'))

            module_name = input_sprx_content["modules"][0]["name"]

            if module_name not in json_list:
                json_list.append(module_name)
                print("Module : " + module_name + " - Generating Stub for this prx!\n")

                for module_library in input_sprx_content["modules"][0]["libraries"]:
                    lib_name = module_library["name"]
                    lib_is_export = module_library["is_export"]
                    lib_symbols = module_library["symbols"]

                    if lib_is_export:
                        print("detected exported library: " + lib_name)
                        generate_assembly(lib_name, lib_symbols, args.output_path)
    
    generate_makefile(args.output_path)

if __name__ == "__main__":
    # Execute only if run as a script
    main()