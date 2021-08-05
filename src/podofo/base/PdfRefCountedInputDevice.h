/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_REF_COUNTED_INPUT_DEVICE_H
#define PDF_REF_COUNTED_INPUT_DEVICE_H

#include "PdfDefines.h"

namespace PoDoFo {

class PdfInputDevice;

// TODO: Remove-me! Just use std::shared_ptr<PdfInputDevice>

/**
 * A reference counted input device object
 * which is closed as soon as the last
 * object having access to it is deleted.
 */
class PODOFO_API PdfRefCountedInputDevice
{
public:
    /** Created an empty reference counted input device object
     *  The input device will be initialize to nullptr
     */
    PdfRefCountedInputDevice();

    /** Create a new PdfRefCountedInputDevice which reads from a file.
     *
     *  \param filename a filename
     */
    PdfRefCountedInputDevice(const std::string_view& filename);

    /** Create a new PdfRefCountedInputDevice which operates on a in memory buffer
     *
     *  \param buffer pointer to the buffer
     *  \param len length of the buffer
     */
    PdfRefCountedInputDevice(const char* buffer, size_t len);

    /** Create a new PdfRefCountedInputDevice from an PdfInputDevice
     *
     *  \param device the input device. It will be owned and deleted by this object.
     */
    PdfRefCountedInputDevice(PdfInputDevice* device);

    /** Copy an existing PdfRefCountedInputDevice and increase
     *  the reference count
     *  \param rhs the PdfRefCountedInputDevice to copy
     */
    PdfRefCountedInputDevice(const PdfRefCountedInputDevice& rhs);

    /** Decrease the reference count and close the file
     *  if this is the last owner
     */
    ~PdfRefCountedInputDevice();

    /** Get access to the file handle
     *  \returns the file handle
     */
    PdfInputDevice* Device() const;

    /** Copy an existing PdfRefCountedFile and increase
     *  the reference count
     *  \param rhs the PdfRefCountedFile to copy
     *  \returns the copied object
     */
    const PdfRefCountedInputDevice& operator=(const PdfRefCountedInputDevice& rhs);

private:
    /** Detach from the reference counted file
     */
    void Detach();

private:
    typedef struct
    {
        PdfInputDevice* Device;
        unsigned RefCount;
    } TRefCountedInputDevice;

    TRefCountedInputDevice* m_Device;
};

};

#endif // PDF_REF_COUNTED_INPUT_DEVICE_H


