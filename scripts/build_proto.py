#!/usr/bin/env python3
import fileinput
import argparse
import os

MAGIC_STRING_CS = "if (!pb::UnknownFieldSet.MergeFieldFrom(ref _unknownFields, input)) {"
FIXED_STRING_CS = "_unknownFields = pb::UnknownFieldSet.MergeFieldFrom(_unknownFields, input);"

MAGIC_STRING_PBC = '#include "external/'
FIXED_STRING_PBC = '#include "'

def fixFile(fileName):
    data = [ ]

    with open(fileName, "r") as file:
        data = file.readlines()

    for i in range(0, len(data), 1):
        if MAGIC_STRING_CS in data[i]:
            data[i] = data[i].replace(MAGIC_STRING_CS, FIXED_STRING_CS)

            if "return;" not in data[i+1]:
                print("error error error, return statement not on line " + (i+1))

            data[i+1] = ""

            if "}" not in data[i+2]:
                print("error error error closing bracket not found")

            data[i+2] = ""

        elif MAGIC_STRING_PBC in data[i]:
            data[i] = data[i].replace(MAGIC_STRING_PBC, FIXED_STRING_PBC)

    with open(fileName, "w") as file:
        file.writelines(data)

if __name__== "__main__":
    inputDirectory = "."
    miraDirectory = "."
    noMvPbcFiles = False

    parser = argparse.ArgumentParser(prog="build_proto.py")
    parser.add_argument("--inputDir", help="input directory (default: .)")
    parser.add_argument("--outputDir", help="output directory (default is input directory)")
    parser.add_argument("--miraDir", help="mira directory (default: .)")
    parser.add_argument("--noPbcMv", help="option to skip the move of the protobuf files (default: False)")

    args = parser.parse_args()

    # if the user provided a input directory then use that instead of .
    if args.inputDir:
        inputDirectory = args.inputDir

    outputDirectory = inputDirectory

    if args.outputDir:
        outputDirectory = args.outputDir

    if args.miraDir:
        miraDirectory = args.miraDir

    pbFileList = [ ]
    pbFileListString = ""

    # Find all of the proto files in the current running directory
    for file in os.listdir(inputDirectory):
        if file.endswith(".proto"):
            fullFilePath = os.path.join(inputDirectory, file)
            pbFileList.append(fullFilePath)
            pbFileListString += fullFilePath + " "

    os.system("protoc-c --c_out=. " + pbFileListString)
    os.system("protoc --csharp_out=" + outputDirectory + " " + pbFileListString)

    csharpFileList = [ ]
    pbcFileList = [ ]
    pbcPathList = [ ]

    for file in os.listdir(outputDirectory):
        if file.endswith(".cs"):
            fullFilePath = os.path.join(outputDirectory, file)
            csharpFileList.append(fullFilePath)

    for file in os.listdir(inputDirectory):
        if "pb-c" in file:
            pbcFileList.append(file)
            fullFilePath = os.path.join(inputDirectory, file)
            pbcPathList.append(fullFilePath)

    for file in (csharpFileList + pbcPathList):
        fixFile(file)

    if noMvPbcFiles is False:
        # Move fixed protobuf files
        for file in pbcFileList:
            sourcePath = os.path.join(inputDirectory, file)
            destinationPath = ""

            if "debugger" in file:
                destinationPath = os.path.join(miraDirectory, ("src/Plugins/Debugger/" + file))
            elif "filemanager" in file:
                destinationPath = os.path.join(miraDirectory, ("src/Plugins/FileManager/" + file))
            elif "rpc" in file:
                destinationPath = os.path.join(miraDirectory, ("src/Messaging/Rpc/" + file))

            #Create destination paths dynamicly
            if not os.path.exists(os.path.dirname(destinationPath)):
                 os.makedirs(os.path.dirname(destinationPath))
            
            os.replace(sourcePath, destinationPath)

    print("completed")
