#include <Python.h>
#include <structmember.h>

// ----------------------------------------------------
// Part #1: Reproducer of cyclic GC issue
// ----------------------------------------------------

typedef struct {
    PyObject_HEAD
    PyObject *nested;
} wrapper;

static int wrapper_init(PyObject *self, PyObject *args, PyObject *kwds) {
    printf("wrapper tp_init called.\n");
    return 0;
}

static void wrapper_dealloc(PyObject *self) {
    printf("wrapper tp_dealloc called.\n");
    PyObject_GC_UnTrack(self);
    PyTypeObject *tp = Py_TYPE(self);
    Py_CLEAR(((wrapper *) self)->nested);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static int wrapper_traverse(PyObject *self, visitproc visit, void *arg) {
    printf("wrapper tp_traverse called.\n");
    Py_VISIT(((wrapper *) self)->nested);
    return 0;
}

static int wrapper_clear(PyObject *self) {
    printf("wrapper tp_clear called.\n");
    Py_CLEAR(((wrapper *) self)->nested);
    return 0;
}

static PyMemberDef wrapper_members[] = {
    { "nested", T_OBJECT, offsetof(wrapper, nested), 0, NULL },
    { NULL , 0, 0, 0, NULL }
};

static PyType_Slot wrapper_slots[] = {
    { Py_tp_init, wrapper_init },
    { Py_tp_dealloc, wrapper_dealloc },
    { Py_tp_traverse, wrapper_traverse },
    { Py_tp_clear, wrapper_clear },
    { Py_tp_members, wrapper_members },
    { 0, NULL }
};

static PyType_Spec wrapper_spec = {
    .name = "pypy_issues.wrapper",
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .slots = wrapper_slots,
    .basicsize = (int) sizeof(wrapper),
    .itemsize = 0
};

// ----------------------------------------------------
// Part #2: Reproducer of name inconsistencies
// ----------------------------------------------------

static int func_init(PyObject *self, PyObject *args, PyObject *kwds) {
    return 0;
}

static PyObject *func_get_name(PyObject *self, void* unused) {
    return PyUnicode_FromString("my_name");
}

static PyObject *func_get_qualname(PyObject *self, void* unused) {
    return PyUnicode_FromString("my_qualname");
}

static PyObject *func_get_module(PyObject *self, void* unused) {
    return PyUnicode_FromString("my_module");
}

#if PY_VERSION_HEX < 0x03090000
// PyGetSetDef entry for __module__ is ignored in Python 3.8
PyObject *func_getattro(PyObject *self, PyObject *name_) {
    const char *name = PyUnicode_AsUTF8AndSize(name_, NULL);

    if (!name)
        return NULL;
    else if (strcmp(name, "__module__") == 0)
        return func_get_module(self, NULL);
    else
        return PyObject_GenericGetAttr(self, name_);
}
#endif

static PyGetSetDef func_getset[] = {
    { "__name__", func_get_name, NULL, NULL, NULL },
    { "__qualname__", func_get_qualname, NULL, NULL, NULL },
    { "__module__", func_get_module, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

static PyType_Slot func_slots[] = {
    { Py_tp_init, func_init },
    { Py_tp_getset, func_getset },
#if PY_VERSION_HEX < 0x03090000
    { Py_tp_getattro, (void *) func_getattro },
#endif
    { 0, NULL }
};

static PyType_Spec func_spec = {
    .name = "pypy_issues.func",
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = func_slots,
    .basicsize = (int) sizeof(PyObject),
    .itemsize = 0
};

// ----------------------------------------------------
// Part #3: Reproducer of method vector call issue
// ----------------------------------------------------

static PyObject* method_call(PyObject* self, PyObject* arg) {
    PyObject *value = PyLong_FromLong(1234);
    if (!value)
        return NULL;

    PyObject *name = PyUnicode_FromString("my_method");
    if (!name) {
        Py_DECREF(value);
        return NULL;
    }

    PyObject *args[2] = { arg, value };
    size_t nargsf = 2 | PY_VECTORCALL_ARGUMENTS_OFFSET;

    PyObject *result = PyObject_VectorcallMethod(
        name, args, nargsf, NULL);

    Py_DECREF(value);
    Py_DECREF(name);

    return result;
}

static PyObject* method_call_kw(PyObject* self, PyObject* arg) {
    PyObject *value = NULL,
             *name = NULL,
             *kwnames = NULL,
             *kw_name = NULL,
             *kw_value = NULL,
             *result = NULL;

    value = PyLong_FromLong(1234);
    if (!value)
        goto leave;

    name = PyUnicode_FromString("my_method");
    if (!name)
        goto leave;

    kwnames = PyTuple_New(1);
    if (!kwnames)
        goto leave;

    kw_name = PyUnicode_InternFromString("foo");
    if (!kw_name)
        goto leave;
    PyTuple_SET_ITEM(kwnames, 0, kw_name);

    kw_value = PyUnicode_FromString("bar");
    if (!kw_value)
        goto leave;

    PyObject *args[3] = { arg, value, kw_value };
    size_t nargsf = 2 | PY_VECTORCALL_ARGUMENTS_OFFSET;

    result = PyObject_VectorcallMethod(
        name, args, nargsf, kwnames);

leave:
    Py_XDECREF(value);
    Py_XDECREF(name);
    Py_XDECREF(kwnames);
    Py_XDECREF(kw_value);

    return result;
}

// ----------------------------------------------------

static PyObject* get_name(PyObject* self, PyObject* arg) {
    if (PyType_CheckExact(arg)) {
        return PyUnicode_FromString(((PyTypeObject *) arg)->tp_name);
    } else {
        PyErr_SetString(PyExc_TypeError, "expected a type object");
        return NULL;
    }
}

// ----------------------------------------------------

struct PyMethodDef pypy_issues_methods[] = {
    { "method_call", (PyCFunction) method_call, METH_O, NULL },
    { "method_call_kw", (PyCFunction) method_call_kw, METH_O, NULL },
    { "get_name", (PyCFunction) get_name, METH_O, NULL },
    { NULL, NULL, 0, NULL},
};

// ----------------------------------------------------

static PyModuleDef pypy_issues_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "pypy_issues",
    .m_doc = "Reproducer for miscellaneous PyPy issues",
    .m_size = -1,
    .m_methods = pypy_issues_methods
};

PyMODINIT_FUNC
PyInit_pypy_issues(void)
{
    PyObject *m = PyModule_Create(&pypy_issues_module);
    if (m == NULL)
        return NULL;

    PyObject *wrapper = PyType_FromSpec(&wrapper_spec);

    if (!wrapper || PyModule_AddObject(m, "wrapper", wrapper) < 0) {
        Py_XDECREF(wrapper);
        Py_DECREF(m);
        return NULL;
    }

    PyObject *func = PyType_FromSpec(&func_spec);

    if (!func || PyModule_AddObject(m, "func", func) < 0) {
        Py_XDECREF(func);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
