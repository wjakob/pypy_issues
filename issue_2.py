from pypy_issues import callable

c = callable()
assert c() == 1234
