#!/usr/bin/python

# Importing all test scripts to be run in this main script
#import mic_test
#import servo_test
#import stepper_test
#import valve_test

# Importing Necessary Libraries
import subprocess
import os
import time
# for windows
os.system('cls')
time.sleep(2)
# for linux unknown TODO
# Different Functions

functions = ["Mic", "Servos", "Steppers", "Valves", "All", "End Testing"]
functionNames =  ["mic_test.py", "servo_test.py", "stepper_test.py", "valve_test.py"]

def runPythonScirpt(num):
    print("You have selected: ", functions[num], "test. Running Test...")
    subprocess.call(['python', functionNames[num]])
    print("-----------------------------------------------------------------")

def menuRunner():
    # Calls for an infinite loop that keeps executing
    # until an exception occurs
    while True:
        print("Welcome to your integrated testing experience. Choose an option from the list below\nto get started with your unit testing!")
        print("Enter the cooresponding number to run the test. You may also run all the test scripts from this menu\nTest...\n")
       # getting length of list
        length = len(functions)
        # Iterating the index
        # same as 'for i in range(len(list))'
        for i in range(length):
            print(i, ": ", functions[i])

        try:
            test4num = int(input("Please select a single number from the list above: " ))

        except ValueError:
            # The cycle will go on until validation
            print("Error! Try again.")

        # When successfully converted to an integer,
        # the loop will end.
        else:
            break

    length2 = len(functions)
    if test4num < length2:
        if functions[test4num] != "All" and functions[test4num] != "End Testing": #ensures we are not meant to stop the testing protocol, and that we are not meant to test them all
            runPythonScirpt(test4num)
        elif functions[test4num] == "All":
            print("Running All Tests")
            functionsListLength = len(functionNames)
            for j in range(functionsListLength):
                runPythonScirpt(j)
        else:
            print("Exiting. You now have the high ground")
            exit()
    else:
        print("Number too large. Please re-run with an appropriate value")
  
def resetWorkingDirectory():
    currentWorking = os.getcwd()
    print("Current Working Directory: ", currentWorking)
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    currentWorking = os.getcwd()
    print("New Working Directory: ", currentWorking)

# ----------------------------- Script Process to Run -------------------------------------------------
resetWorkingDirectory()
menuRunner()