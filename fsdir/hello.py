
# Variables and data types
name = "John"
age = 25
is_student = True

# Printing output
print("Hello, " + name)
print("You are " + str(age) + " years old")
print("Are you a student? " + str(is_student))

# Conditional statements
if age >= 18:
	print("You are an adult")
else:
	print("You are a minor")

"""
I am a multiline comment
"""

# Looping
for i in range(5):
	print("Count: " + str(i))

# Lists
fruits = ["apple", "banana", "orange"]
print("My favorite fruit is " + fruits[0])

# Functions
def greet(name):
	print("Hello, " + name)

greet("Alice")
