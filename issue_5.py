# Expected output (CPython 3.9)
#
#     wrapper tp_init called.
#     wrapper tp_traverse called.
#     wrapper tp_traverse called.
#     wrapper tp_clear called.
#     wrapper tp_dealloc called.
#
# Actual output (PyPy 3.9 tip)
#
#     wrapper tp_init called.
#

from pypy_issues import wrapper
import gc

# Create an unreferenced cycle
a = wrapper()
a.nested = a
del a

# Collect garbage
gc.collect()
gc.collect()
gc.collect()
