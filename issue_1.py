from pypy_issues import heap_type
import gc

for i in range(100):
    heap_type()
    gc.collect()
