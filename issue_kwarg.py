from pypy_issues import my_object, my_callable
import gc

def f():
    o = my_object()
    c = my_callable()

    # Comment try/except block to fix refleak
    try:
        c(my_kwarg=o)
    except:
        pass # Ignore exception

    del o

    # GC 3 times in function scope for good measure!
    gc.collect()
    gc.collect()
    gc.collect()

f()

# And once more here in the global scope :-)
gc.collect()
gc.collect()
gc.collect()
