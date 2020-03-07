#!/usr/bin/env python3
import fileinput
import argparse
import os

MAGIC_STRING = "if (!pb::UnknownFieldSet.MergeFieldFrom(ref _unknownFields, input)) {"
FIXED_STRING = "_unknownFields = pb::UnknownFieldSet.MergeFieldFrom(_unknownFields, input);"

def fixCSharp(fileName):
    data = [ ]

    with open(fileName, "r") as file:
        data = file.readlines()
    
    for i in range(0, len(data), 1):
        if MAGIC_STRING in data[i]:
            data[i] = data[i].replace(MAGIC_STRING, FIXED_STRING)
            
            if "return;" not in data[i+1]:
                print("error error error, return statement not on line " + (i+1))
            
            data[i+1] = ""

            if "}" not in data[i+2]:
                print("error error error closing bracket not found")
            
            data[i+2] = ""
    
    with open(fileName, "w") as file:
        file.writelines(data)

if __name__== "__main__":
    inputDirectory = "."
    parser = argparse.ArgumentParser(prog="build_proto.py")
    parser.add_argument("--inputDir", help="input directory (default: .)")

    args = parser.parse_args()

    # if the user provided a input directory then use that instead of .
    if args.inputDir:
        inputDirectory = args.inputDir
    
    pbFileList = [ ]
    pbFileListString = ""

    # Find all of the proto files in the current running directory
    for file in os.listdir(inputDirectory):
        if file.endswith(".proto"):
            fullFilePath = os.path.join(inputDirectory, file)
            pbFileList.append(fullFilePath)
            pbFileListString += fullFilePath + " "
    

    os.system("protoc-c --c_out=" + inputDirectory + " " + pbFileListString)
    os.system("protoc --csharp_out=" + inputDirectory + " " + pbFileListString)

    csharpFileList = [ ] 
    for file in os.listdir(inputDirectory):
        if file.endswith(".cs"):
            fullFilePath = os.path.join(inputDirectory, file)
            csharpFileList.append(fullFilePath)
    
    for file in csharpFileList:
        fixCSharp(file)
    
    print("completed")
