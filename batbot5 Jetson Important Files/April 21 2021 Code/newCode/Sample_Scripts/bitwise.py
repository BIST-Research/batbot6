# Basic code to come to understand the binary operations
# of Python, and how they interface with the Servo library

# Testing binary value A and int value B

print('Testing Bitwise Operations')

inputA = int('0101',2)
inputB = int(64)

print ("Before shifting " + str(inputA) + " " + bin(inputA))
print ("After shifting in binary: " + bin(inputA << 1))
print ("After shifting in decimal: " + str(inputA << 1))
print ("Binary value is: " + bin(inputB))