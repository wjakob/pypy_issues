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

typedef struct {
    PyObject_HEAD
    PyObject* (*vectorcall)(PyObject *, PyObject * const*, size_t, PyObject *);
} callable;


PyObject* my_vectorcall(PyObject *self, PyObject * const* args, size_t nargs,
                        PyObject *kwnames) {
    PyErr_SetString(PyExc_TypeError, "oops, an exception!");
    return NULL;
}

static int callable_init(PyObject *self, PyObject *args, PyObject *kwds) {
    ((callable *) self)->vectorcall = my_vectorcall;
    return 0;
}

static struct PyMemberDef callable_members[] = {
    // Supported starting with Python 3.9
    { "__vectorcalloffset__", T_PYSSIZET,
      (Py_ssize_t) offsetof(callable, vectorcall), READONLY, NULL },
     { NULL, 0, 0, 0, NULL }
};

static PyType_Slot callable_slots[] = {
    { Py_tp_init, callable_init },
    { Py_tp_members, callable_members },
    { Py_tp_call, (void *) PyVectorcall_Call },
    { 0, NULL }
};

static PyType_Spec callable_spec = {
    .name = "pypy_issues.callable",
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_VECTORCALL,
    .slots = callable_slots,
    .basicsize = (int) sizeof(callable),
    .itemsize = 0
};


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

    PyObject *func = PyType_FromSpec(&callable_spec);

    if (!func || PyModule_AddObject(m, "callable", func) < 0) {
        Py_XDECREF(func);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
