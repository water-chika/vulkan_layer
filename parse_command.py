import xml.etree.ElementTree as ET

tree = ET.parse('Vulkan-Headers/registry/vk.xml')
root = tree.getroot()
print("{")
for command in root.findall('commands/command'):
    ret_type = command.find('proto/type')
    name = command.find('proto/name')
    params = command.findall('param')
    if (name != None and ret_type != None and params != None):
        print("    ",end="")
        print("{")
        print("        ",end="")
        print("\"",ret_type.text, "\"", ", ", "\"", name.text, "\"", ",",sep="")
        print("        ",end="")
        print("{")
        for param in params:
            param_type = param.find('type').text
            param_name = param.find('name').text
            print("            ",end="")
            print("{", end="")
            print("\"",param_type,"\"", ", ", "\"", param_name, "\"",sep="", end="")
            print("},")
        print("        ",end="")
        print("}")
        print("    ",end="")
        print("},")
print("}")