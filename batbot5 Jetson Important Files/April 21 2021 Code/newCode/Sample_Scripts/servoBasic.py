# Basic code to come to understand the binary operations
# of Python, and how they interface with the Servo library

# Testing input degrees to binary val
# Converts the input degrees to a binary equivalent based on the 
# opcode spec

MAX_BINARY = int(0b111111)
DEGREES_TOTAL = int(180)

print('The specified degrees:')
x = input()
if int(x) <= 180 and int(x) >= 0:
    print('Input degrees is: ' + str(x) + '. Beginning Conversion')
    print('Max Bianry is is: ' + str(MAX_BINARY))
    print('Total degrees possible is: ' + str(DEGREES_TOTAL))
    y = bin(round((int(x)/DEGREES_TOTAL) * MAX_BINARY))
    print ("Final binary value for designated input: " + str(y))
else:
    print('Number invalid. Consider a number in the range of 0 to ' + str(DEGREES_TOTAL))
