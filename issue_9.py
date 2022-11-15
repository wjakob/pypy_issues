from pypy_issues import wrapper, callable
import gc

def f():
    w = wrapper()

    # Comment try/except block to fix refleak
    try:
        callable()(w)
    except:
        pass # Ignore exception

    del w

    # GC 3 times in callabletion scope for good measure!
    gc.collect()
    gc.collect()
    gc.collect()

f()

# And once more here in the global scope :-)
gc.collect()
gc.collect()
gc.collect()
