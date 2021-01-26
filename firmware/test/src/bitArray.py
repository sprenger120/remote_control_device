# Splits up a repeating binary sequency to 8 bit groups with 0b infront of it
# for usage in arrays

inp = "00111110100"


# will contain data in network tranmission order 
# least signficant bit of first channel -> most significant of last channel
rawData = ""

for n in range(0,16):
    rawData += inp[::-1]

#print(len(oneBinary))

newline = 1
oneByte = ""
for n in range(0, len(rawData)):
    oneByte += rawData[n]
    if len(oneByte) == 8:
        # uart restores bit order to little endian again 
        # so to emulate every byte needs to be reversed but byte ordering remains the same
        print("0b" + str(oneByte[::-1]) + ", ", end='')
        oneByte = ""
        if newline == 4:
            print("")
            newline = 0
        newline += 1


print("")