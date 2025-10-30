from sys import stdout

TABLE_SIZE = 32 * 16 # Number of steps (brightness)

def write_image():
    blue = "0xff, 0x00, 0x00, "
    green = "0x00, 0xff, 0x00, "
    red = "0x00, 0x00, 0xff, "
    yellow = "0x00, 0xff, 0xff, "
    cyan = "0xff, 0xff, 0x00, "

    stdout.write('color_image = {\n')
    for i in range(0, 16):
        for j in range(0, 32):
            if i < 4:
                stdout.write(f'{red}')
            elif i < 8:
                stdout.write(f'{green}')
            elif i < 12:
                stdout.write(f'{blue}')
            elif i < 14:
                stdout.write(f'{yellow}')
            else:
                stdout.write(f'{cyan}')
        stdout.write('\n')
    stdout.write('};\n')

write_image();
