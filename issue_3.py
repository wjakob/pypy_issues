from pypy_issues import call

def f(value):
    assert value == 1234

call(f)
