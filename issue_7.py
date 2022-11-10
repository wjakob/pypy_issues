from pypy_issues import method_call

class A:
    def __init__(self):
        self.value = 0

    def my_method(self, value):
        self.value = value

a = A()
method_call(a)
assert a.value == 1234
