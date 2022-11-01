#include <Python.h>
#include <structmember.h>

// ----------------------------------------------------
// Part 1: Reproducer of reference counting issue
// https://foss.heptapod.net/pypy/pypy/-/issues/3844
// ----------------------------------------------------

PyObject *heap_type_new(PyTypeObject *tp, PyObject *args,
                        PyObject *kwds) {

    PyObject *obj = PyObject_Malloc(sizeof(PyObject));
    memset(obj, 0, sizeof(PyObject));
    return PyObject_Init(obj, tp);
}

void heap_type_dealloc(PyObject *self) {
    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);

#if PY_VERSION_HEX > 0x03080000
    Py_DECREF(tp);
#endif
}

static PyType_Slot heap_type_slots[] = {
    { Py_tp_new, heap_type_new },
    { Py_tp_dealloc, heap_type_dealloc },
    { 0, NULL }
};

static PyType_Spec heap_type_spec = {
    .name = "pypy_issues.heap_type",
    .flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HEAPTYPE,
    .slots = heap_type_slots,
    .basicsize = sizeof(PyObject),
    .itemsize = 0
};

// ----------------------------------------------------
// Part 2: Reproducer of vector call issue #1
// https://foss.heptapod.net/pypy/pypy/-/issues/3844
// ----------------------------------------------------

typedef struct {
    PyObject_HEAD
    PyObject* (*vectorcall)(PyObject *, PyObject * const*, size_t, PyObject *);
} callable;

PyObject* my_vectorcall(PyObject *self, PyObject * const* args, size_t nargs,
                        PyObject *kwnames) {
    return PyLong_FromLong(1234);
}

static struct PyMemberDef callable_members[] = {
    // Supported starting with Python 3.9
    { "__vectorcalloffset__", T_PYSSIZET,
      (Py_ssize_t) offsetof(callable, vectorcall), READONLY, NULL },
     { NULL, 0, 0, 0, NULL }
 };

int callable_init(PyObject *self, PyObject *args, PyObject *kwds) {
    ((callable *) self)->vectorcall = my_vectorcall;
    return 0;
}

static PyType_Slot callable_slots[] = {
    { Py_tp_init, (void *) callable_init },
    { Py_tp_members, (void *) callable_members },
    { Py_tp_call, (void *) PyVectorcall_Call },
    { 0, NULL }
};

static PyType_Spec callable_spec = {
    .name = "pypy_issues.callable",
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_VECTORCALL,
    .slots = callable_slots
};

// ----------------------------------------------------
// Part #3 Reproducer of vector call issue #2
// https://foss.heptapod.net/pypy/pypy/-/issues/3844
// ----------------------------------------------------

static PyObject* call(PyObject* self, PyObject* arg) {
    PyObject *value = PyLong_FromLong(1234);
    if (!value)
        return NULL;

    PyObject *args[2] = { NULL, value };
    size_t nargsf = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;

    PyObject *result = PyObject_Vectorcall(
        arg, args + 1, nargsf, NULL
    );

    Py_DECREF(value);

    return result;
}

struct PyMethodDef pypy_issues_methods[] = {
    { "call", (PyCFunction) call, METH_O, NULL },
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

    PyObject *heap_type = PyType_FromSpec(&heap_type_spec);

    if (PyModule_AddObject(m, "heap_type", heap_type) < 0) {
        Py_DECREF(heap_type);
        Py_DECREF(m);
        return NULL;
    }

    PyObject *callable = PyType_FromSpec(&callable_spec);

    if (PyModule_AddObject(m, "callable", callable) < 0) {
        Py_DECREF(callable);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}