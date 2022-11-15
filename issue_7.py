from pypy_issues import method_call, method_call_kw

class A:
    def my_method(self, *args, **kwargs):
        self.args = args
        self.kwargs = kwargs
        return "success"

a = A()
assert method_call(a) == "success"
assert a.args == (1234,) and a.kwargs == {}
assert method_call_kw(a) == "success"
assert a.args == (1234,) and a.kwargs == {"foo" : "bar"}


args_value = []
kwargs_value = {}

def unbound_method(*args, **kwargs):
    global args_value, kwargs_value
    args_value = args
    kwargs_value = kwargs
    return "unbound"

a.my_method = unbound_method

assert method_call(a) == "unbound"
assert args_value == (1234,) and kwargs_value == {}
assert method_call_kw(a) == "unbound"
assert args_value == (1234,) and kwargs_value == {"foo" : "bar"}
