#include <Python.h>
#include <iostream>

#include <zxing/common/Counted.h>
#include <zxing/common/GreyscaleLuminanceSource.h>
#include <zxing/Binarizer.h>
#include <zxing/common/HybridBinarizer.h>
#include <zxing/common/GlobalHistogramBinarizer.h>
#include <zxing/DecodeHints.h>
#include <zxing/BarcodeFormat.h>
#include <zxing/BinaryBitmap.h>
#include <zxing/MultiFormatReader.h>
#include <zxing/multi/GenericMultipleBarcodeReader.h>
#include <zxing/ReaderException.h>

using namespace std;
using namespace zxing;
using namespace zxing::multi;

static void log_error(const string &msg) {
    static PyObject *logger = NULL;

    if (logger == NULL) {
        PyObject *logging = PyImport_ImportModuleNoBlock("logging");

        if (logging == NULL) {
            PyErr_SetString(PyExc_ImportError, "Could not import module 'logging'");

            return;
        }

        logger = PyObject_CallMethod(logging, "getLogger", "O", PyUnicode_FromString("zxinglight"));
    }

    PyObject_CallMethod(logger, "error", "O", PyUnicode_FromString(msg.c_str()));
}

static vector<string> *_zxing_read_codes(char *image, int image_size, int width, int height,
                                         int barcode_type, bool try_harder, bool hybrid) {
    try {
        ArrayRef<char> data = ArrayRef<char>(image, image_size);

        Ref<GreyscaleLuminanceSource> source = Ref<GreyscaleLuminanceSource>(
            new GreyscaleLuminanceSource(data, width, height, 0, 0, width, height)
            );

        Ref<Binarizer> binarizer;

        if (hybrid) {
            binarizer = new HybridBinarizer(source);
        } else {
            binarizer = new GlobalHistogramBinarizer(source);
        }

        DecodeHints hints;

        if (barcode_type == 0) {
            hints = DecodeHints(DecodeHints::DEFAULT_HINT);
        } else {
            hints = DecodeHints();
            hints.addFormat(BarcodeFormat((BarcodeFormat::Value) barcode_type));
        }

        hints.setTryHarder(try_harder);

        Ref<BinaryBitmap> bitmap(new BinaryBitmap(binarizer));

        MultiFormatReader delegate;
        GenericMultipleBarcodeReader reader(delegate);
        vector<Ref<Result> > results = reader.decodeMultiple(bitmap, hints);

        vector<string> *codes = new vector<string>();

        for (const Ref<Result> &result : results) {
            codes->push_back(result->getText()->getText());
        }

        return codes;
    } catch (const ReaderException &e) {
        log_error((string) "zxing::ReaderException: " + e.what());
    } catch (const zxing::IllegalArgumentException &e) {
        log_error((string) "zxing::IllegalArgumentException: " + e.what());
    } catch (const zxing::Exception &e) {
        log_error((string) "zxing::Exception: " + e.what());
    } catch (const std::exception &e) {
        log_error((string) "std::exception: " + e.what());
    }

    return NULL;
}

static PyObject* zxing_read_codes(PyObject *self, PyObject *args) {
    PyObject *python_image;
    int width, height, barcode_type;
    int try_harder, hybrid;

#if PY_MAJOR_VERSION >= 3
    if (!PyArg_ParseTuple(args, "Siiipp", &python_image, &width, &height, &barcode_type,
                          &try_harder, &hybrid)) {
#else
    if (!PyArg_ParseTuple(args, "Siiiii", &python_image, &width, &height, &barcode_type,
                          &try_harder, &hybrid)) {
#endif
        return NULL;
    }

    char *image;
    Py_ssize_t image_size;

    PyBytes_AsStringAndSize(python_image, &image, &image_size);

    vector<string> *results = _zxing_read_codes(
        image, image_size, width, height, barcode_type, try_harder, hybrid
        );

    PyObject *codes = PyList_New(0);

    if (results != NULL) {
        for (const string &code : *results) {
            PyList_Append(codes, PyUnicode_FromString(code.c_str()));
        }

        delete results;
    }

    return codes;
}

static PyMethodDef zxinglight_functions[] = {
    { "zxing_read_codes", zxing_read_codes, METH_VARARGS, NULL },
    { NULL }
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef zxinglight_moduledef = {
    PyModuleDef_HEAD_INIT,
    "_zxinglight",
    NULL,
    -1,
    zxinglight_functions,
};
#endif

#if PY_MAJOR_VERSION >= 3
    #define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
#else
    #define MOD_INIT(name) PyMODINIT_FUNC init##name(void)
#endif

MOD_INIT(_zxinglight) {
#if PY_MAJOR_VERSION >= 3
    return PyModule_Create(&zxinglight_moduledef);
#else
    Py_InitModule3("_zxinglight",
        zxinglight_functions, NULL);
#endif
}
