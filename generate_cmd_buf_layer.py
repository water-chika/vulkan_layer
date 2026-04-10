import xml.etree.ElementTree as ET
import sys

def generate(file):
    tree = ET.parse(file)
    root = tree.getroot()
    for command in root.findall('commands/command'):
        ret_type = command.find('proto/type')
        name = command.find('proto/name')
        params = command.findall('param')
        if (name != None and ret_type != None and params != None and 'cmdbufferlevel' in command.attrib):
            print(ret_type.text, name.text, "(", end="")
            first_param = True;
            for param in params:
                if not first_param:
                    print(",")
                else:
                    print("")
                    first_param = False
                param_type = param.find('type').text
                param_name = param.find('name').text
                print("    ", param_type, param_name, end="")
            print(") {")
            print("    could_split = false;")
            print("}")

if __name__ == '__main__':
    file = sys.argv[1]
    generate(file)
