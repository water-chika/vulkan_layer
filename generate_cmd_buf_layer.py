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
            if name.text == 'vkCmdRefreshObjectsKHR':
                continue
            print(ret_type.text, name.text[2:], "(", end="", file=output)
            first_param = True;
            param_names = ""
            for param in params:
                if not first_param:
                    print(",", file=output)
                    param_names += ", ";
                else:
                    print("", file=output)
                    first_param = False
                print("   ", "".join(param.itertext()).replace("VkRefreshObjectListKHR", "void"), end="", file=output)
                param_names += param.find("name").text
            print(") {", file=output)
            print("    std::cerr << \"could not split {}\" << std::endl;".format(name.text), file=output)
            print("    could_split = false;", file=output)
            print("    auto next_call = reinterpret_cast<PFN_{0}>(reinterpret_cast<T*>(this)->get_next_device_proc_addr(\"{0}\"));".format(name.text), file=output)
            print("    return next_call({});".format(param_names), file=output)
            print("}", file=output)

def generate_cmd_buf(file, output):
    print("#pragma once", file=output)
    print("namespace water_chika {", file=output)
    print("template<typename T>", file=output)
    print("class cmd_buf_split_info{", file=output)
    print("public:", file=output)
    print("cmd_buf_split_info(bool could_split = true) : could_split{could_split}{}", file=output)
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
