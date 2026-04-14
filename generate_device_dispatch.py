import xml.etree.ElementTree as ET
import sys
import argparse

def should_override(command):
    ret_type = command.find('proto/type')
    name = command.find('proto/name')
    params = command.findall('param')
    names = {
            "vkGetDeviceProcAddr",
            "vkCreateCommandPool"
             }
    return name != None and ret_type != None and params != None and (('cmdbufferlevel' in command.attrib) and name.text != 'vkCmdRefreshObjectsKHR' or (name.text in names))

def generate_dispatch(output):
    tree = ET.parse(file)
    root = tree.getroot()
    for command in root.findall('commands/command'):
        ret_type = command.find('proto/type')
        name = command.find('proto/name')
        params = command.findall('param')
        if should_override(command):
            print("extern \"C\" DLLEXPORT", ret_type.text, "VKAPI_CALL", "water_chika_{}".format(name.text), "(", end="", file=output)
            first_param = True;
            param_names = ""
            for param in params:
                if not first_param:
                    print(",", file=output)
                    param_names += ", ";
                else:
                    print("", file=output)
                    first_param = False
                print("    ", "".join(param.itertext()).replace("VkRefreshObjectListKHR", "void"), end="", file=output)
                param_names += param.find("name").text
            print(") {", file=output)
            if 'cmdbufferlevel' in command.attrib or len(params) > 0 and params[0].find('type').text == 'VkCommandBuffer':
                print("    auto& layer = water_chika_debug_command_buffer_info::g_command_buffers[commandBuffer];", file=output)
            else:
                print("    auto& layer = water_chika_debug_device_layer::g_devices[device];", file=output)
            print("    return layer.{}({});".format(name.text[2:], param_names), file=output)
            print("}", file=output)

def generate_get_device_procs(output):
    print("auto get_device_procs(){", file=output);
    print("    auto funcs = std::unordered_map<std::string, void*>{", file=output);
    tree = ET.parse(file)
    root = tree.getroot()
    for command in root.findall('commands/command'):
        ret_type = command.find('proto/type')
        name = command.find('proto/name')
        params = command.findall('param')
        if should_override(command):
          print("    {\""+name.text+"\", (void*)water_chika_"+name.text+"},", file=output)
    print("    };", file=output)
    print("    return funcs;", file=output)
    print("}", file=output)

def generate_cmd_buf(file, output):
    print("#pragma once", file=output)
    generate_dispatch(output)
    generate_get_device_procs(output)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('registry_xml', type=str)
    parser.add_argument('--output', type=str)
    args = parser.parse_args()
    file = args.registry_xml
    with open(args.output, 'w') as output:
        generate_cmd_buf(file, output)
