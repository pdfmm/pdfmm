/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_VEC_OBJECTS_H
#define PDF_VEC_OBJECTS_H

#include <set>
#include <list>
#include "PdfDefines.h"
#include "PdfReference.h"

namespace PoDoFo {

class PdfDocument;
class PdfObject;
class PdfStream;
class PdfVariant;

// Use deque as many insertions are here way faster than with using std::list
// This is especially useful for PDFs like PDFReference17.pdf with
// lots of free objects.
typedef std::deque<PdfReference>                 TPdfReferenceList;
typedef TPdfReferenceList::iterator              TIPdfReferenceList;
typedef TPdfReferenceList::const_iterator        TCIPdfReferenceList;

typedef std::set<uint32_t>                       TPdfObjectNumList;

typedef std::set<PdfReference>                   TPdfReferenceSet;
typedef TPdfReferenceSet::iterator               TIPdfReferenceSet;
typedef TPdfReferenceSet::const_iterator         TCIPdfReferenceSet;

typedef std::list<PdfReference*>                 TReferencePointerList;
typedef TReferencePointerList::iterator          TIReferencePointerList;
typedef TReferencePointerList::const_iterator    TCIReferencePointerList;

typedef std::vector<TReferencePointerList>       TVecReferencePointerList;
typedef TVecReferencePointerList::iterator       TIVecReferencePointerList;
typedef TVecReferencePointerList::const_iterator TCIVecReferencePointerList;

typedef std::vector<PdfObject*>      TVecObjects;
typedef TVecObjects::iterator        TIVecObjects;
typedef TVecObjects::const_iterator  TCIVecObjects;

/** A STL vector of PdfObjects. I.e. a list of PdfObject classes.
 *  The PdfParser will read the PdfFile into memory and create
 *  a PdfVecObjects of all dictionaries found in the PDF file.
 *
 *  The PdfWriter class contrary creates a PdfVecObjects internally
 *  and writes it to a PDF file later with an appropriate table of
 *  contents.
 *
 *  This class contains also advanced functions for searching of PdfObject's
 *  in a PdfVecObject.
 */
class PODOFO_API PdfVecObjects final
{
    friend class PdfWriter;
    friend class PdfDocument;
    friend class PdfParser;
    friend class PdfObjectStreamParser;

public:
    // An incomplete set of container typedefs, just enough to handle
    // the begin() and end() methods we wrap from the internal vector.
    // TODO: proper wrapper iterator class.
    typedef TVecObjects::iterator iterator;
    typedef TVecObjects::const_iterator const_iterator;

    /** Every observer of PdfVecObjects has to implement this interface.
     */
    class PODOFO_API Observer
    {
        friend class PdfVecObjects;
    public:
        virtual ~Observer() { }

        virtual void WriteObject(const PdfObject& obj) = 0;

        /** Called whenever appending to a stream is started.
         *  \param pStream the stream object the user currently writes to.
         */
        virtual void BeginAppendStream(const PdfStream& stream) = 0;

        /** Called whenever appending to a stream has ended.
         *  \param pStream the stream object the user currently writes to.
         */
        virtual void EndAppendStream(const PdfStream& stream) = 0;

        virtual void Finish() = 0;
    };

    /** This class is used to implement stream factories in PoDoFo.
     */
    class PODOFO_API StreamFactory
    {
    public:
        virtual ~StreamFactory() { }

        /** Creates a stream object
         *
         *  \param pParent parent object
         *
         *  \returns a new stream object
         */
        virtual PdfStream* CreateStream(PdfObject& parent) = 0;
    };

private:
    typedef std::vector<Observer*>        TVecObservers;
    typedef TVecObservers::iterator       TIVecObservers;
    typedef TVecObservers::const_iterator TCIVecObservers;

public:
    PdfVecObjects(PdfDocument& document);

    ~PdfVecObjects();

    /** Enable/disable object numbers re-use.
     *  By default object numbers re-use is enabled.
     *
     *  \param bCanReuseObjectNumbers if true, free object numbers can be re-used when creating new objects.
     *
     *  If set to false, the list of free object numbers is automatically cleared.
     */
    void SetCanReuseObjectNumbers(bool canReuseObjectNumbers);

    /** Removes all objects from the vector
     *  and resets it to the default state.
     *
     *  If SetAutoDelete is true all objects are deleted.
     *  All observers are removed from the vector.
     *
     *  \see SetAutoDelete
     *  \see IsAutoDelete
     */
    void Clear();

    /**
     *  \returns the size of the internal vector
     */
    size_t GetSize() const;

    /**
     *  \returns the highest object number in the vector
     */
    size_t GetObjectCount() const { return m_ObjectCount; }

    /** Finds the object with the given reference in m_vecOffsets
     *  and returns a pointer to it if it is found. Throws a PdfError
     *  exception with error code ePdfError_NoObject if no object was found
     *  \param ref the object to be found
     *  \returns the found object
     *  \throws PdfError(ePdfError_NoObject)
     */
    PdfObject& MustGetObject(const PdfReference& ref) const;

    /** Finds the object with the given reference in m_vecOffsets
     *  and returns a pointer to it if it is found.
     *  \param ref the object to be found
     *  \returns the found object or nullptr if no object was found.
     */
    PdfObject* GetObject(const PdfReference& ref) const;

    /** Remove the object with the given object and generation number from the list
     *  of objects.
     *  The object is returned if it was found. Otherwise nullptr is returned.
     *  The caller has to delete the object by himself.
     *
     *  \param ref the object to be found
     *  \param bMarkAsFree if true the removed object reference is marked as free object
     *                     you will always want to have this true
     *                     as invalid PDF files can be generated otherwise
     *  \returns The removed object.
     */
    std::unique_ptr<PdfObject> RemoveObject(const PdfReference& ref, bool markAsFree = true);

    /** Remove the object with the iterator it from the vector and return it
     *  \param it the object to remove
     *  \returns the removed object
     */
    std::unique_ptr<PdfObject> RemoveObject(const TIVecObjects& it);

    /** Creates a new object and inserts it into the vector.
     *  This function assigns the next free object number to the PdfObject.
     *
     *  \param pszType optional value of the /Type key of the object
     *  \returns PdfObject pointer to the new PdfObject
     */
    PdfObject* CreateDictionaryObject(const std::string_view& type = { });

    /** Creates a new object (of type rVariants) and inserts it into the vector.
     *  This function assigns the next free object number to the PdfObject.
     *
     *  \param rVariant value of the PdfObject
     *  \returns PdfObject pointer to the new PdfObject
     */
    PdfObject* CreateObject(const PdfVariant& variant);

    /**
     *  Renumbers all objects according to there current position in the vector.
     *  All references remain intact.
     *  Warning! This function is _very_ calculation intensive.
     *
     *  \param pTrailer the trailer object
     *  \param pNotDelete a list of object which must not be deleted
     *  \param bDoGarbageCollection enable garbage collection, which deletes
     *         all objects that are not reachable from the trailer. This might be slow!
     *
     *  \see CollectGarbage
     */
    void RenumberObjects(PdfObject& trailer, TPdfReferenceSet* notDelete = nullptr, bool doGarbageCollection = false);

    /**
     * Sort the objects in the vector based on their object and generation numbers
     */
    void Sort();

    /**
     * Set the maximum number of elements Reserve() will work for (to fix
     * CVE-2018-5783) which is called with a value from the PDF in the parser.
     * The default is from Table C.1 in section C.2 of PDF32000_2008.pdf
     * (PDF 1.7 standard free version).
     * This sets a static variable, so don't use from multiple threads
     * (without proper locking).
     * \param size Number of elements to allow to be reserved
     */
    void SetMaxReserveSize(size_t size) { m_MaxReserveSize = size; }

    /**
     * Gets the maximum number of elements Reserve() will work for (to fix
     * CVE-2018-5783) which is called with a value from the PDF in the parser.
     * The default is from Table C.1 in section C.2 of PDF32000_2008.pdf
     * (PDF 1.7 standard free version): 8388607.
     */
    size_t GetMaxReserveSize() const { return m_MaxReserveSize; }

    /**
     * Causes the internal vector to reserve space for size elements.
     * \param size reserve space for that much elements in the internal vector
     */
    void Reserve(size_t size);

    /** Get a set with all references of objects that the passed object
     *  depends on.
     *  \param pObj the object to calculate all dependencies for
     *  \param pList write the list of dependencies to this list
     *
     */
    void GetObjectDependencies(const PdfObject& obj, TPdfReferenceList& list) const;


    /** Attach a new observer
     *  \param pObserver to attach
     */
    void Attach(Observer* observer);

    /** Detach an observer.
     *
     *  \param pObserver observer to detach
     */
    void Detach(Observer* observer);

    /** Sets a StreamFactory which is used whenever CreateStream is called.
     *
     *  \param pFactory a stream factory or nullptr to reset to the default factory
     */
    void SetStreamFactory(StreamFactory* factory);

    /** Creates a stream object
     *  This method is a factory for PdfStream objects.
     *
     *  \param pParent parent object
     *
     *  \returns a new stream object
     */
    PdfStream* CreateStream(PdfObject& parent);

    /** Can be called to force objects to be written to disk.
     *
     *  \param obj a PdfObject that should be written to disk.
     */
    void WriteObject(PdfObject& obj);

    /** Call whenever a document is finished
     */
    void Finish();

    /** Every stream implementation has to call this in BeginAppend
     *  \param pStream the stream object that is calling
     */
    void BeginAppendStream(const PdfStream& stream);

    /** Every stream implementation has to call this in EndAppend
     *  \param pStream the stream object that is calling
     */
    void EndAppendStream(const PdfStream& stream);

    PdfObject*& operator[](size_t index);

    /** Get the last object in the vector
     *  \returns the last object in the vector or nullptr
     *           if the vector is empty.
     */
    PdfObject* GetBack();

    /**
     * Deletes all objects that are not references by other objects
     * besides the trailer (which references the root dictionary, which in
     * turn should reference all other objects).
     *
     * \param pTrailer trailer object of the PDF
     *
     * Warning this might be slow!
     */
    void CollectGarbage(PdfObject& trailer);

    /**
     * Set the object count so that the object described this reference
     * is contained in the object count.
     *
     * \param rRef reference of newly added object
     */
    void SetObjectCount(const PdfReference& ref);

public:
    /** Iterator pointing at the beginning of the vector
     *  \returns beginning iterator
     */
    TCIVecObjects begin() const;

    /** Iterator pointing at the end of the vector
     *  \returns ending iterator
     */
    TCIVecObjects end() const;

    size_t size() const;

public:
    /** \returns a pointer to a PdfDocument that is the
     *           parent of this vector.
     *           Might be nullptr if the vector has no parent.
     */
    inline PdfDocument& GetDocument() const { return *m_Document; }

    /**
     *  \returns whether can re-use free object numbers when creating new objects.
     */
    inline bool GetCanReuseObjectNumbers() const { return m_CanReuseObjectNumbers; }

    /** \returns a list of free references in this vector
     */
    inline const TPdfReferenceList& GetFreeObjects() const { return m_lstFreeObjects; }

private:
    /** Insert an object into this vector so that
     *  the vector remains sorted w.r.t.
     *  the ordering based on object and generation numbers
     *  m_bObjectCount will be increased for the object.
     *
     *  \param pObj pointer to the object you want to insert
     */
    void AddObject(PdfObject* obj);

    /** Push an object with the givent reference. If one is existing, it will be replaced
     */
    void PushObject(const PdfReference& reference, PdfObject* obj);

    /** Mark a reference as unused so that it can be reused for new objects.
     *
     *  Add the object only if the generation is the allowed range
     *
     *  \param rReference the reference to reuse
     *  \returns true if the object was succesfully added
     *
     *  \see AddFreeObject
     */
    bool TryAddFreeObject(const PdfReference& reference);

    /** Mark a reference as unused so that it can be reused for new objects.
     *
     *  Add the object and increment the generation number. Add the object
     *  only if the generation is the allowed range
     *
     *  \param rReference the reference to reuse
     *  \returns the generation of the added free object
     *
     *  \see AddFreeObject
     */
    int32_t SafeAddFreeObject(const PdfReference& reference);

    /** Mark a reference as unused so that it can be reused for new objects.
     *  \param rReference the reference to reuse
     *
     *  \see GetCanReuseObjectNumbers
     */
    void AddFreeObject(const PdfReference& reference);

private:
    void addNewObject(PdfObject* obj);

    /**
     * \returns the next free object reference
     */
    PdfReference getNextFreeObject();

    int32_t tryAddFreeObject(uint32_t objnum, uint32_t gennum);

    /**
     * Create a list of all references that point to the object
     * for each object in this vector.
     * \param pList write all references to this list
     */
    void buildReferenceCountVector(TVecReferencePointerList& list);
    void insertReferencesIntoVector(const PdfObject& obj, TVecReferencePointerList& list);

    /** Assumes that the PdfVecObjects is sorted
     */
    void insertOneReferenceIntoVector(const PdfObject& obj, TVecReferencePointerList& list);

    /** Delete all objects from the vector which do not have references to them selves
     *  \param pList must be a list created by BuildReferenceCountVector
     *  \param pTrailer must be the trailer object so that it is not deleted
     *  \param pNotDelete a list of object which must not be deleted
     *  \see BuildReferenceCountVector
     */
    void garbageCollection(TVecReferencePointerList& list, PdfObject& trailer, TPdfReferenceSet* notDelete = nullptr);

private:
    PdfVecObjects(const PdfVecObjects&) = delete;
    PdfVecObjects& operator=(const PdfVecObjects&) = delete;

private:
    PdfDocument* m_Document;
    bool m_CanReuseObjectNumbers;
    size_t m_ObjectCount;
    bool m_sorted;
    TVecObjects m_vector;

    TVecObservers m_vecObservers;
    TPdfReferenceList m_lstFreeObjects;
    TPdfObjectNumList m_lstUnavailableObjects;

    StreamFactory* m_StreamFactory;

    std::string m_SubsetPrefix;		    // Prefix for BaseFont and FontName of subsetted font
    static size_t m_MaxReserveSize;
};

};

#endif // PDF_VEC_OBJECTS_H
