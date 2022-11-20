#include <Python.h>
#include <structmember.h>

// ----------------------------------------------------

static int my_object_init(PyObject *self, PyObject *args, PyObject *kwds) {
    printf("my_object tp_init called.\n");
    return 0;
}

static void my_object_dealloc(PyObject *self) {
    printf("my_object tp_dealloc called.\n");
    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}


static PyType_Slot my_object_slots[] = {
    { Py_tp_init, my_object_init },
    { Py_tp_dealloc, my_object_dealloc },
    { 0, NULL }
};

static PyType_Spec my_object_spec = {
    .name = "pypy_issues.my_object",
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = my_object_slots,
    .basicsize = (int) sizeof(PyObject),
    .itemsize = 0
};

// ----------------------------------------------------

typedef struct {
    PyObject_HEAD
    PyObject* (*vectorcall)(PyObject *, PyObject * const*, size_t, PyObject *);
} my_callable;


PyObject* my_vectorcall(PyObject *self, PyObject * const* args, size_t nargs,
                        PyObject *kwnames) {
    printf("my_callable(*args, **kwargs) called -- raising an exception\n");
    PyErr_SetString(PyExc_TypeError, "oops, an exception!");
    return NULL;
}

static int my_callable_init(PyObject *self, PyObject *args, PyObject *kwds) {
    ((my_callable *) self)->vectorcall = my_vectorcall;
    return 0;
}

static struct PyMemberDef my_callable_members[] = {
    // Supported starting with Python 3.9
    { "__vectorcalloffset__", T_PYSSIZET,
      (Py_ssize_t) offsetof(my_callable, vectorcall), READONLY, NULL },
     { NULL, 0, 0, 0, NULL }
};

static PyType_Slot my_callable_slots[] = {
    { Py_tp_init, my_callable_init },
    { Py_tp_members, my_callable_members },
    { Py_tp_call, (void *) PyVectorcall_Call },
    { 0, NULL }
};

static PyType_Spec my_callable_spec = {
    .name = "pypy_issues.my_callable",
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_VECTORCALL,
    .slots = my_callable_slots,
    .basicsize = (int) sizeof(my_callable),
    .itemsize = 0
};


static PyModuleDef pypy_issues_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "pypy_issues",
    .m_doc = "Reproducer for miscellaneous PyPy issues",
    .m_size = -1
};

PyMODINIT_FUNC
PyInit_pypy_issues(void)
{
    PyObject *m = PyModule_Create(&pypy_issues_module);
    if (m == NULL)
        return NULL;

    PyObject *my_object = PyType_FromSpec(&my_object_spec);

    if (!my_object || PyModule_AddObject(m, "my_object", my_object) < 0) {
        Py_XDECREF(my_object);
        Py_DECREF(m);
        return NULL;
    }

    PyObject *func = PyType_FromSpec(&my_callable_spec);

    if (!func || PyModule_AddObject(m, "my_callable", func) < 0) {
        Py_XDECREF(func);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
