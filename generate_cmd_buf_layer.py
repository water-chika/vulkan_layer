import xml.etree.ElementTree as ET
import sys
import argparse

def generate(file, output):
    tree = ET.parse(file)
    root = tree.getroot()
    for command in root.findall('commands/command'):
        ret_type = command.find('proto/type')
        name = command.find('proto/name')
        params = command.findall('param')
        if (name != None and ret_type != None and params != None and 'cmdbufferlevel' in command.attrib):
            print(ret_type.text, name.text, "(", end="", file=output)
            first_param = True;
            for param in params:
                if not first_param:
                    print(",", file=output)
                else:
                    print("", file=output)
                    first_param = False
                print("    ", "".join(param.itertext()).replace("VkRefreshObjectListKHR", "void"), end="", file=output)
            print(") {", file=output)
            print("    could_split = false;", file=output)
            print("}", file=output)

def generate_cmd_buf(file, output):
    print("#pragma once", file=output)
    print("namespace water_chika {", file=output)
    print("class cmd_buf_split_info {", file=output)
    print("public:", file=output)
    print("cmd_buf_split_info(bool could_split) : could_split{could_split}{}", file=output)
    generate(file, output)
    print("protected:", file=output)
    print("bool could_split;", file=output)
    print("};//class cmd_buf_info", file=output)
    print("}//namespace water_chika", file=output)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('registry_xml', type=str)
    parser.add_argument('--output', type=str)
    args = parser.parse_args()
    file = args.registry_xml
    with open(args.output, 'w') as output:
        generate_cmd_buf(file, output)
