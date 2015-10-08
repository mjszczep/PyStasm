// py_wrapper.cpp: Stasm Python bindings
//
// "THIS IS GONNA GO REALLY WELL"
// - Matthew Szczepankiewicz, September 14, 2015

#if _MSC_VER >= 1800
  #define HAVE_ROUND 1 // Handle warning C4273: don't redefine round with Python
#endif

#include <Python.h>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
#include "stasm.h"

#if PY_MAJOR_VERSION >= 3
  #define PyStr_AS_STRING(ob) PyUnicode_AsUTF8(ob)
#else
  #define PyStr_AS_STRING(ob) PyString_AsString(ob)
#endif

static const char StasmException_doc[]	= "Stasm library error.";
static PyObject *StasmException, *datadir_default;

static const char trace_error[]		= "trace must be set to True or False.";
static const char multiface_error[]	= "multiface must be set to True or False.";
static const char minwidth_error[]	= "Minimum face width must be between 1 and 100.";
static const char imarray_error[]	= "Invalid image array.";
static const char imarray_dim_error[]	= "Image must be a 2D array.";
static const char landmark_error[]	= "Invalid landmark array.";
static const char landmark_dim_error[]	= "Landmarks must be a 2D array.";

PyObject* landmarks_to_PyArray( // Convert landmarks array to numpy array
	float*	landmarks,	// in
	int	num_landmarks)	// in
{
	const int nd = 2;
	npy_intp dims[nd] = { num_landmarks, 2 };
	PyObject* retArray = PyArray_SimpleNewFromData(nd, dims, NPY_FLOAT, landmarks);
	PyArray_ENABLEFLAGS((PyArrayObject*)retArray, NPY_ARRAY_OWNDATA);

	return retArray;
}

/*
 * Returns image data from a PyObject representing a numpy array.
 *
 * In:		array_obj	-	Numpy array with image data
 * Out:		width		-	Image width
 * Out:		height		-	Image height
 *
 * Returns:	char* pointing to the array data;
 * 			NULL if array is invalid
 */
const char* PyArray_to_image(
	PyObject*	array_obj,
	int* 		width,
	int*		height)
{
	PyArrayObject* img_array = (PyArrayObject*)
		PyArray_FROM_OTF(array_obj, NPY_UINT8, NPY_ARRAY_IN_ARRAY);

	if (img_array == NULL)
	{
		PyErr_SetString(PyExc_TypeError, imarray_error);
		return NULL;
	}

	if (PyArray_NDIM(img_array) != 2)
	{
		PyErr_SetString(PyExc_TypeError, imarray_dim_error);
		return NULL;
	}

	*height = (int)PyArray_DIM(img_array, 0);
	*width = (int)PyArray_DIM(img_array, 1);
	const char* img_data = PyArray_BYTES(img_array);

	Py_DECREF(img_array);

	return img_data;
}

static const char init_doc[] = 
"init(datadir=stasm.DATADIR, trace=False) -> None\n"
"\n"
"Initialize Stasm.\n"
"\n"
"Parameters:\n"
"    datadir: Path to directory containing Haar cascade XML files.\n"
"    trace: Set to True to trace to stdout and stasm.log for debugging.\n";

static PyObject* Py_init(            
	PyObject*	self,
	PyObject*	args,
	PyObject*	kwargs)
{
	const char*	datadir	= PyStr_AS_STRING(datadir_default);
	int		trace	= 0;

	static const char* kwlist[] = {"datadir", "trace", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|si:init",
					const_cast<char**>(kwlist), &datadir,
					&trace))
		return NULL;

	if (trace != 0 && trace != 1)
	{
		PyErr_SetString(PyExc_TypeError, trace_error);
		return NULL;
	}

	if (!stasm_init(datadir, trace))
	{
		PyErr_SetString(StasmException, stasm_lasterr());
		return NULL;
	}

	Py_RETURN_NONE;
}

static const char open_image_doc[] = 
"open_image(image, debugpath=\"\", multiface=False, minwidth=10) -> None\n"
"\n"
"Load an image for processing with Stasm.\n"
"\n"
"Parameters:\n"
"    image: ndarray containing gray image data.\n"
"    debugpath: Image file location used for debugging.\n"
"    multiface: Only search for one face if False; allow\n"
"               multiple faces if True.\n"
"    minwidth: Minimum face width, as a percentage of image width.\n";

static PyObject* Py_open_image(
	PyObject*	self,
	PyObject*	args,
	PyObject*	kwargs)
{
	PyObject*	img_obj;
	const char*	debugpath	= "";
	int		multiface	= 0;
	int		minwidth	= 10;

	static const char* kwlist[] = { "image", "debugpath", "multiface", "minwidth", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|sii:open_image",
					const_cast<char**>(kwlist), &img_obj,
					&debugpath, &multiface, &minwidth))
		return NULL;

	int width, height;
	const char* img_data = PyArray_to_image(img_obj, &width, &height);
	if (img_data == NULL)
	{
		PyErr_SetString(PyExc_TypeError, imarray_error);
		return NULL;
	}

	if (multiface != 0 && multiface != 1)
	{
		PyErr_SetString(PyExc_TypeError, multiface_error);
		return NULL;
	}

	if (minwidth < 1 || minwidth > 100)
	{
		PyErr_SetString(PyExc_ValueError, minwidth_error);
		return NULL;
	}

	if (!stasm_open_image(img_data, width, height,
			      debugpath, multiface, minwidth))
	{
		PyErr_SetString(StasmException, stasm_lasterr());
		return NULL;
	}

	Py_RETURN_NONE;
}

static const char search_auto_doc[] =
"search_auto() -> numpy.ndarray\n"
"\n"
"Call repeatedly to find all faces.\n"
"\n"
"Returns:\n"
"    ndarray with shape (stasm.NLANDMARKS, 2) containing face\n"
"    landmarks as [x,y] pairs, or empty ndarray if none found.\n";

static PyObject* Py_search_auto(
	PyObject*	self,
	PyObject*	)
{
	int foundface;
	float* landmarks;

	landmarks = new float[stasm_NLANDMARKS * 2];

	if (!stasm_search_auto(&foundface, landmarks))
	{
		PyErr_SetString(StasmException, stasm_lasterr());
		delete[] landmarks;
		return NULL;
	}

	int num_landmarks = foundface ? stasm_NLANDMARKS : 0;
	return landmarks_to_PyArray(landmarks, num_landmarks);
}

static const char search_single_doc[] =
"search_single(image, debugpath=\"\", datadir=stasm.DATADIR) -> numpy.ndarray\n"
"\n"
"Opens and searches image for a single face.\n"
"\n"
"Parameters:\n"
"    image: ndarray containing gray image data.\n"
"    debugpath: Image file location used for debugging.\n"
"    datadir: Path to directory containing Haar cascade XML files.\n"
"\n"
"Returns:\n"
"    ndarray with shape (stasm.NLANDMARKS, 2) containing face\n"
"    landmarks as [x,y] pairs, or empty ndarray if none found.\n";

static PyObject* Py_search_single(
	PyObject*	self,
	PyObject*	args,
	PyObject*	kwargs)
{
	PyObject*	img_obj;
	const char*	debugpath	= "";
	const char*	datadir		= PyStr_AS_STRING(datadir_default);

	static const char* kwlist[] = { "image", "debugpath", "datadir", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|ss:search_single",
					const_cast<char**>(kwlist), &img_obj,
					&debugpath, &datadir))
		return NULL;

	int width, height;
	const char* img_data = PyArray_to_image(img_obj, &width, &height);
	if (img_data == NULL)
	{
		PyErr_SetString(PyExc_TypeError, imarray_error);
		return NULL;
	}

	int foundface;
	float* landmarks = new float[stasm_NLANDMARKS * 2];

	if (!stasm_search_single(&foundface, landmarks, img_data, width,
				 height, debugpath, datadir))
	{
		PyErr_SetString(StasmException, stasm_lasterr());
		delete[] landmarks;
		return NULL;
	}

	int landmarks_found = foundface ? stasm_NLANDMARKS : 0;
	return landmarks_to_PyArray(landmarks, landmarks_found);
}

static const char search_pinned_doc[] =
"search_pinned(pinned, img, debugpath=\"\") -> numpy.ndarray\n"
"\n"
"Find landmarks with no OpenCV face detection. Call after\n"
"pinning some points.\n"
"\n"
"Parameters:\n"
"    pinned: ndarray containing pinned points as [x,y] pairs.\n"
"    img: ndarray containing gray image data.\n"
"    debugpath: Image file location used for debugging.\n"
"\n"
"Returns:\n"
"    ndarray with shape (stasm.NLANDMARKS, 2) containing face\n"
"    landmarks as [x,y] pairs, or empty ndarray if none found.\n";

static PyObject* Py_search_pinned(
	PyObject*	self,
	PyObject*	args)
{
	PyObject*	pinned_obj;
	PyObject*	img_obj;
	const char*	debugpath = "";

	if (!PyArg_ParseTuple(args, "OO|s:search_pinned", &pinned_obj,
				&img_obj, &debugpath))
		return NULL;

	int width, height;
	const char* img_data = PyArray_to_image(img_obj, &width, &height);
	if (img_data == NULL)
	{
		PyErr_SetString(PyExc_TypeError, imarray_error);
		return NULL;
	}

	PyArrayObject* pinned_array = (PyArrayObject*)PyArray_FROM_OTF(
				    pinned_obj, NPY_FLOAT, NPY_ARRAY_IN_ARRAY);
	if (pinned_array == NULL)
		return NULL;

	float* pinned = (float*)PyArray_DATA(pinned_array);
	float* landmarks = new float[stasm_NLANDMARKS * 2];

	Py_DECREF(pinned_array);

	if (!stasm_search_pinned(landmarks, pinned, img_data,
				width, height, debugpath))
	{
		PyErr_SetString(StasmException, stasm_lasterr());
		delete[] landmarks;
		return NULL;
	}

	return landmarks_to_PyArray(landmarks, stasm_NLANDMARKS);
}

static const char lasterr_doc[] =
"lasterr() -> str\n"
"\n"
"Get last stasm error.\n"
"\n"
"Returns:\n"
"    String describing last error.\n";

static PyObject* Py_lasterr(
	PyObject*	self,
	PyObject*	)
{
	const char* lasterr = stasm_lasterr();
	PyObject* ret = Py_BuildValue("s", lasterr);
	return ret;
}

static const char force_points_into_image_doc[] =
"force_points_into_image(landmarks, width, height) -> numpy.ndarray\n"
"\n"
"Force landmarks into image boundary.\n"
"\n"
"Parameters:\n"
"    landmarks: ndarray containing landmarks as [x,y] pairs.\n"
"    img: ndarray containing image to force points to.\n"
"\n"
"Returns:\n"
"    ndarray containing landmarks within image boundary.\n";

static PyObject* Py_force_points_into_image(
	PyObject*	self,
	PyObject*	args)
{
	PyObject*	landmarks_obj;
	PyObject*	img_obj;

	if (!PyArg_ParseTuple(args, "OO:force_points_into_image",
				&landmarks_obj, &img_obj))
		return NULL;

	PyArrayObject* landmarks_array = (PyArrayObject*)
		PyArray_FROM_OTF(landmarks_obj, NPY_FLOAT, NPY_ARRAY_IN_ARRAY);
	if (landmarks_array == NULL)
	{
		PyErr_SetString(PyExc_TypeError, landmark_error);
		return NULL;
	}

	if (PyArray_NDIM(landmarks_array) != 2)
	{
		PyErr_SetString(PyExc_TypeError, landmark_dim_error);
		return NULL;
	}

	int width, height;
	if (PyArray_to_image(img_obj, &width, &height) == NULL)
		return NULL;

	PyObject* retArray = PyArray_Copy(landmarks_array);
	Py_DECREF(landmarks_array);
	float* landmarks = (float*)PyArray_DATA((PyArrayObject*)retArray);
	stasm_force_points_into_image(landmarks, width, height);

	return retArray;
}

static const char convert_shape_doc[] =
"convert_shape(landmarks, format) -> numpy.ndarray\n"
"\n"
"Convert stasm.NLANDMARKS points to external format.\n"
"\n"
"Parameters:\n"
"    landmarks: ndarray containing landmarks as [x,y] pairs.\n"
"    format: One of the following options:\n"
"            stasm.SHAPE17,\n"
"            stasm.BIOID,\n"
"            stasm.AR,\n"
"            stasm.XM2VTS,\n"
"            stasm.MUCT76\n"
"\n"
"Returns:\n"
"    ndarray containing landmarks in specified format.\n"
"\n"
"Notes:\n"
"    Any number of points can be supplied if stasm.SHAPE17 is specified\n"
"    as the format type. For all others, landmarks must contain exactly\n"
"    77 points or convert_shape will return an empty ndarray.\n";

static PyObject* Py_convert_shape(
	PyObject*	self,
	PyObject*	args)
{
	PyObject*	landmarks_obj;
	int		format;

	if (!PyArg_ParseTuple(args, "Oi:convert_shape", &landmarks_obj, &format))
		return NULL;

	PyArrayObject* landmarks_array = (PyArrayObject*)
			PyArray_FROM_OTF(landmarks_obj, NPY_FLOAT, NPY_ARRAY_IN_ARRAY);
	if (landmarks_array == NULL)
	{
		PyErr_SetString(PyExc_TypeError, landmark_error);
		return NULL;
	}

	if (PyArray_NDIM(landmarks_array) != 2)
	{
		PyErr_SetString(PyExc_TypeError, landmark_dim_error);
		return NULL;
	}

	PyObject* retArray = PyArray_Copy(landmarks_array);
	Py_DECREF(landmarks_array);
	float* landmarks = (float*)PyArray_DATA((PyArrayObject*)retArray);
	stasm_convert_shape(landmarks, format);

	return retArray;
}

static const char module_doc[] = 
"Python wrapper for finding features in faces.\n"
"\n"
"Stasm is a C++ software library for finding features in faces.\n"
"For more information, visit http://www.milbo.users.sonic.net/stasm/\n";

static PyMethodDef module_methods[] = {
	{ "init", (PyCFunction)Py_init, METH_VARARGS | METH_KEYWORDS, init_doc },
	{ "open_image", (PyCFunction)Py_open_image, METH_VARARGS | METH_KEYWORDS, open_image_doc },
	{ "search_auto", Py_search_auto, METH_NOARGS, search_auto_doc },
	{ "search_single", (PyCFunction)Py_search_single, METH_VARARGS | METH_KEYWORDS, search_single_doc },
	{ "search_pinned", Py_search_pinned, METH_VARARGS, search_pinned_doc },
	{ "lasterr", Py_lasterr, METH_NOARGS, lasterr_doc },
	{ "force_points_into_image", Py_force_points_into_image, METH_VARARGS, force_points_into_image_doc },
	{ "convert_shape", Py_convert_shape, METH_VARARGS, convert_shape_doc },
	{ NULL, NULL, 0, NULL }
};

// See http://python3porting.com/cextensions.html
#if PY_MAJOR_VERSION >= 3
  #define MOD_ERROR_VAL NULL
  #define MOD_SUCCESS_VAL(val) val
  #define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
  #define MOD_DEF(ob, name, doc, methods) \
          static struct PyModuleDef moduledef = { \
            PyModuleDef_HEAD_INIT, name, doc, -1, methods, }; \
          ob = PyModule_Create(&moduledef);
  #define PyErr_NEWEXCEPTIONWITHDOC(name, doc) \
          PyErr_NewExceptionWithDoc(name, doc, NULL, NULL)
#else
  #define MOD_ERROR_VAL
  #define MOD_SUCCESS_VAL(val)
  #define MOD_INIT(name) PyMODINIT_FUNC init##name(void)
  #define MOD_DEF(ob, name, doc, methods) \
          ob = Py_InitModule3(name, methods, doc);
  #define PyErr_NEWEXCEPTIONWITHDOC(name, doc) \
          PyErr_NewExceptionWithDoc((char*)name, (char*)doc, NULL, NULL)
#endif

MOD_INIT(_stasm)
{
	PyObject *m, *m_import;
	MOD_DEF(m, "_stasm", module_doc, module_methods);

	if (m == NULL)
		return MOD_ERROR_VAL;

	m_import = PyImport_ImportModule("stasm");
	if (m_import == NULL)
		return MOD_ERROR_VAL;
	datadir_default = PyObject_GetAttrString(m_import, "DATADIR");
	Py_DECREF(m_import);

	StasmException = PyErr_NEWEXCEPTIONWITHDOC("_stasm.StasmException", StasmException_doc);
	Py_INCREF(StasmException);

	/* Note: API Docs for module constants must be updated manually. */
	int error = 0;
	error |= PyModule_AddObject(m, "StasmException", StasmException);
	error |= PyModule_AddObject(m, "DATADIR", datadir_default);
	error |= PyModule_AddIntConstant(m, "NLANDMARKS", stasm_NLANDMARKS);
	error |= PyModule_AddIntConstant(m, "SHAPE17", 17);
	error |= PyModule_AddIntConstant(m, "BIOID", 20);
	error |= PyModule_AddIntConstant(m, "AR", 22);
	error |= PyModule_AddIntConstant(m, "XM2VTS", 68);
	error |= PyModule_AddIntConstant(m, "MUCT76", 76);
	error |= PyModule_AddStringConstant(m, "STASM_VERSION", stasm_VERSION);
	if (error)
		return MOD_ERROR_VAL;

	import_array();

	return MOD_SUCCESS_VAL(m);
}

