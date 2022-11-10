# Expected output (CPython 3.9)
#
#     func().__module__   = my_module
#     func().__name__     = my_name
#     func().__qualname__ = my_qualname
#
# Actual output (PyPy 3.9 tip)
#
#     func().__module__   = pypy_issues
#     func().__name__     = my_name
#     Traceback (most recent call last):
#       File "/home/wjakob/pypy_issues/issue_6.py", line 21, in <module>
#         print("func(): __qualname__ = %s" % f.__qualname__)
#     AttributeError: 'func' object has no attribute '__qualname__'

from pypy_issues import func

f = func()
print("func().__module__   = %s" % f.__module__)
print("func().__name__     = %s" % f.__name__)
print("func().__qualname__ = %s" % f.__qualname__)
