/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_OUTPUT_DEVICE_H
#define PDF_OUTPUT_DEVICE_H

#include <ostream>
#include <fstream>

#include "PdfStreamDeviceBase.h"
#include "PdfOutputStream.h"

namespace mm {

class PDFMM_API OutputStreamDevice : virtual public StreamDeviceBase, public OutputStream
{
protected:
    OutputStreamDevice();
    OutputStreamDevice(bool init);

protected:
    void checkWrite() const override;
};

};

#endif // PDF_OUTPUT_DEVICE_H
