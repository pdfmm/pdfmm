/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>
#include "TestExtension.h"

#include <sstream>

constexpr const char* PODOFO_TEST_PAGE_KEY = "PoDoFoTestPageNumber";
constexpr unsigned PODOFO_TEST_NUM_PAGES = 100;

using namespace std;
using namespace mm;

static void appendChildNode(PdfObject& parent, PdfObject& child);
static bool isPageNumber(PdfPage& page, unsigned number);
static vector<PdfPage*> createSamplePages(PdfMemDocument& doc, unsigned pageCount);
static vector<PdfObject*> createNodes(PdfMemDocument& doc, unsigned nodeCount);
static void createEmptyKidsTree(PdfMemDocument& doc);
static void createNestedArrayTree(PdfMemDocument& doc);
static void createTestTreePoDoFo(PdfMemDocument& doc);
static void createCyclicTree(PdfMemDocument& doc, bool createCycle);
static void createTestTreeCustom(PdfMemDocument& doc);
static void testGetPages(PdfMemDocument& doc);
static void testInsert(PdfMemDocument& doc);
static void testDeleteAll(PdfMemDocument& doc);
static void testGetPagesReverse(PdfMemDocument& doc);

TEST_CASE("testEmptyDoc")
{
    PdfMemDocument doc;

    // Empty document must have page count == 0
    REQUIRE(doc.GetPageTree().GetPageCount() == 0);

    // Retrieving any page from an empty document must be NULL
    ASSERT_THROW_WITH_ERROR_CODE(doc.GetPageTree().GetPage(0), PdfErrorCode::PageNotFound);
}

TEST_CASE("testCyclicTree")
{
    for (unsigned pass = 0; pass < 2; pass++)
    {
        PdfMemDocument doc;
        createCyclicTree(doc, pass == 1);
        //doc.Write(pass==0?"tree_valid.pdf":"tree_cyclic.pdf");
        for (unsigned pagenum = 0; pagenum < doc.GetPageTree().GetPageCount(); pagenum++)
        {
            if (pass == 0)
            {
                // pass 0:
                // valid tree without cycles should yield all pages
                PdfPage& page = doc.GetPageTree().GetPage(pagenum);
                REQUIRE(isPageNumber(page, pagenum));
            }
            else
            {
                // pass 1:
                // cyclic tree must throw exception to prevent infinite recursion
                ASSERT_THROW_WITH_ERROR_CODE(doc.GetPageTree().GetPage(pagenum), PdfErrorCode::PageNotFound);
            }
        }
    }
}

TEST_CASE("testEmptyKidsTree")
{
    PdfMemDocument doc;
    createEmptyKidsTree(doc);
    //doc.Write("tree_zerokids.pdf");
    for (unsigned pagenum = 0; pagenum < doc.GetPageTree().GetPageCount(); pagenum++)
    {
        PdfPage& page = doc.GetPageTree().GetPage(pagenum);
        REQUIRE(isPageNumber(page, pagenum));
    }
}

TEST_CASE("testNestedArrayTree")
{
    PdfMemDocument doc;
    createNestedArrayTree(doc);
    //doc.Write("tree_nested_array.pdf");
    for (unsigned pagenum = 0; pagenum < doc.GetPageTree().GetPageCount(); pagenum++)
    {
        (void)doc.GetPageTree().GetPage(pagenum);
    }
}

TEST_CASE("testCreateDelete")
{
    PdfMemDocument doc;
    PdfPage* page;
    PdfPainter painter;

    // create font
    auto font = doc.GetFontManager().GetFont("Arial");
    if (font == nullptr)
        FAIL("Coult not find Arial font");

    // write 1. page
    page = doc.GetPageTree().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
    painter.SetCanvas(page);
    painter.SetFont(font);
    painter.GetTextState().SetFontSize(16.0);
    painter.DrawText(200, 200, "Page 1");
    painter.FinishDrawing();
    REQUIRE(doc.GetPageTree().GetPageCount() == 1);

    // write 2. page
    page = doc.GetPageTree().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
    painter.SetCanvas(page);
    painter.DrawText(200, 200, "Page 2");
    painter.FinishDrawing();
    REQUIRE(doc.GetPageTree().GetPageCount() == 2);

    // try to delete second page, index is 0 based 
    doc.GetPageTree().DeletePage(1);
    REQUIRE(doc.GetPageTree().GetPageCount() == 1);

    // write 3. page
    page = doc.GetPageTree().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
    painter.SetCanvas(page);
    painter.DrawText(200, 200, "Page 3");
    painter.FinishDrawing();
    REQUIRE(doc.GetPageTree().GetPageCount() == 2);
}

TEST_CASE("testGetPagesCustom")
{
    PdfMemDocument doc;
    createTestTreeCustom(doc);
    testGetPages(doc);
}

TEST_CASE("testGetPagesPoDoFo")
{
    PdfMemDocument doc;
    createTestTreePoDoFo(doc);
    testGetPages(doc);
}

TEST_CASE("testGetPagesReverseCustom")
{
    PdfMemDocument doc;
    createTestTreeCustom(doc);
    testGetPagesReverse(doc);
}

TEST_CASE("testGetPagesReversePoDoFo")
{
    PdfMemDocument doc;
    createTestTreePoDoFo(doc);
    testGetPagesReverse(doc);
}

TEST_CASE("testInsertCustom")
{
    PdfMemDocument doc;
    createTestTreeCustom(doc);
    testInsert(doc);
}

TEST_CASE("testInsertPoDoFo")
{
    PdfMemDocument doc;
    createTestTreePoDoFo(doc);
    testInsert(doc);
}

TEST_CASE("testDeleteAllCustom")
{
    PdfMemDocument doc;
    createTestTreeCustom(doc);
    testDeleteAll(doc);
}

TEST_CASE("testDeleteAllPoDoFo")
{
    PdfMemDocument doc;
    createTestTreePoDoFo(doc);
    testDeleteAll(doc);
}

void testGetPages(PdfMemDocument& doc)
{
    for (unsigned i = 0; i < PODOFO_TEST_NUM_PAGES; i++)
    {
        auto& page = doc.GetPageTree().GetPage(i);
        REQUIRE(isPageNumber(page, i));
    }

    // Now delete first page 
    doc.GetPageTree().DeletePage(0);

    for (unsigned i = 0; i < PODOFO_TEST_NUM_PAGES - 1; i++)
    {
        auto& page = doc.GetPageTree().GetPage(i);
        REQUIRE(isPageNumber(page, i + 1));
    }

    // Now delete any page
    constexpr unsigned DELETED_PAGE = 50;
    doc.GetPageTree().DeletePage(DELETED_PAGE);

    for (unsigned i = 0; i < PODOFO_TEST_NUM_PAGES - 2; i++)
    {
        auto& page = doc.GetPageTree().GetPage(i);
        if (i < DELETED_PAGE)
            REQUIRE(isPageNumber(page, i + 1));
        else
            REQUIRE(isPageNumber(page, i + 2));
    }
}

void testGetPagesReverse(PdfMemDocument& doc)
{
    for (int i = PODOFO_TEST_NUM_PAGES - 1; i >= 0; i--)
    {
        unsigned index = (unsigned)i;
        auto& page = doc.GetPageTree().GetPage(index);
        REQUIRE(isPageNumber(page, index));
    }

    // Now delete first page 
    doc.GetPageTree().DeletePage(0);

    for (int i = PODOFO_TEST_NUM_PAGES - 2; i >= 0; i--)
    {
        unsigned index = (unsigned)i;
        auto& page = doc.GetPageTree().GetPage(index);
        REQUIRE(isPageNumber(page, index + 1));
    }
}

void testInsert(PdfMemDocument& doc)
{
    const unsigned INSERTED_PAGE_FLAG = 1234;
    const unsigned INSERTED_PAGE_FLAG1 = 1234 + 1;
    const unsigned INSERTED_PAGE_FLAG2 = 1234 + 2;

    auto page = doc.GetPageTree().InsertPage(0, PdfPage::CreateStandardPageSize(PdfPageSize::A4));
    page->GetObject().GetDictionary().AddKey(PODOFO_TEST_PAGE_KEY,
        static_cast<int64_t>(INSERTED_PAGE_FLAG));

    // Find inserted page (beginning)
    REQUIRE(isPageNumber(doc.GetPageTree().GetPage(0), INSERTED_PAGE_FLAG));

    // Find old first page
    REQUIRE(isPageNumber(doc.GetPageTree().GetPage(1), 0));

    // Insert at end 
    page = doc.GetPageTree().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
    page->GetObject().GetDictionary().AddKey(PODOFO_TEST_PAGE_KEY,
        static_cast<int64_t>(INSERTED_PAGE_FLAG1));

    REQUIRE(isPageNumber(doc.GetPageTree().GetPage(doc.GetPageTree().GetPageCount() - 1),
        INSERTED_PAGE_FLAG1));

    // Insert in middle
    page = doc.GetPageTree().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
    page->GetObject().GetDictionary().AddKey(PODOFO_TEST_PAGE_KEY,
        static_cast<int64_t>(INSERTED_PAGE_FLAG2));

    const unsigned INSERT_POINT = 50;
    doc.GetPageTree().InsertPage(INSERT_POINT, PdfPage::CreateStandardPageSize(PdfPageSize::A4));

    REQUIRE(isPageNumber(doc.GetPageTree().GetPage(INSERT_POINT + 1), INSERTED_PAGE_FLAG2));
}

void testDeleteAll(PdfMemDocument& doc)
{
    for (unsigned i = 0; i < PODOFO_TEST_NUM_PAGES; i++)
    {
        doc.GetPageTree().DeletePage(0);
        REQUIRE(doc.GetPageTree().GetPageCount() == PODOFO_TEST_NUM_PAGES - (i + 1));
    }
    REQUIRE(doc.GetPageTree().GetPageCount() == 0);
}

void createTestTreePoDoFo(PdfMemDocument& doc)
{
    for (unsigned i = 0; i < PODOFO_TEST_NUM_PAGES; i++)
    {
        PdfPage* page = doc.GetPageTree().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
        page->GetObject().GetDictionary().AddKey(PODOFO_TEST_PAGE_KEY, static_cast<int64_t>(i));
        REQUIRE(doc.GetPageTree().GetPageCount() == i + 1);
    }
}

void createTestTreeCustom(PdfMemDocument& doc)
{
    constexpr unsigned COUNT = PODOFO_TEST_NUM_PAGES / 10;
    PdfObject& root = doc.GetPageTree().GetObject();
    PdfArray rootKids;

    for (unsigned z = 0; z < COUNT; z++)
    {
        PdfObject* node = doc.GetObjects().CreateDictionaryObject("Pages");
        PdfArray nodeKids;

        for (unsigned i = 0; i < COUNT; i++)
        {
            PdfPage* page = doc.GetPageTree().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
            page->GetObject().GetDictionary().AddKey(PODOFO_TEST_PAGE_KEY,
                static_cast<int64_t>(z * COUNT + i));

            //printf("Creating page %i z=%i i=%i\n", z * COUNT + i, z, i );
            nodeKids.Add(page->GetObject().GetIndirectReference());
        }

        node->GetDictionary().AddKey("Kids", nodeKids);
        node->GetDictionary().AddKey("Count", static_cast<int64_t>(COUNT));
        rootKids.Add(node->GetIndirectReference());
    }

    root.GetDictionary().AddKey("Kids", rootKids);
    root.GetDictionary().AddKey("Count", static_cast<int64_t>(PODOFO_TEST_NUM_PAGES));
}

vector<PdfPage*> createSamplePages(PdfMemDocument& doc, unsigned pageCount)
{
    // create font
    auto font = doc.GetFontManager().GetFont("Arial");
    if (font == nullptr)
        FAIL("Coult not find Arial font");

    vector<PdfPage*> pages(pageCount);
    for (unsigned i = 0; i < pageCount; ++i)
    {
        pages[i] = doc.GetPageTree().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
        pages[i]->GetObject().GetDictionary().AddKey(PODOFO_TEST_PAGE_KEY, static_cast<int64_t>(i));

        PdfPainter painter;
        painter.SetCanvas(pages[i]);
        painter.SetFont(font);
        painter.GetTextState().SetFontSize(16.0);
        ostringstream os;
        os << "Page " << i + 1;
        painter.DrawText(200, 200, os.str());
        painter.FinishDrawing();
    }

    return pages;
}

vector<PdfObject*> createNodes(PdfMemDocument& doc, unsigned nodeCount)
{
    vector<PdfObject*> nodes(nodeCount);

    for (unsigned i = 0; i < nodeCount; ++i)
    {
        nodes[i] = doc.GetObjects().CreateDictionaryObject("Pages");
        // init required keys
        nodes[i]->GetDictionary().AddKey("Kids", PdfArray());
        nodes[i]->GetDictionary().AddKey("Count", PdfVariant(static_cast<int64_t>(0L)));
    }

    return nodes;
}

void createCyclicTree(PdfMemDocument& doc, bool createCycle)
{
    const unsigned COUNT = 3;

    vector<PdfPage*> pages = createSamplePages(doc, COUNT);
    vector<PdfObject*> nodes = createNodes(doc, 2);

    // manually insert pages into pagetree
    PdfObject& root = doc.GetPageTree().GetObject();

    // tree layout (for !bCreateCycle):
    //
    //    root
    //    +-- node0
    //        +-- node1
    //        |   +-- page0
    //        |   +-- page1
    //        \-- page2

    // root node
    appendChildNode(root, *nodes[0]);

    // tree node 0
    appendChildNode(*nodes[0], *nodes[1]);
    appendChildNode(*nodes[0], pages[2]->GetObject());

    // tree node 1
    appendChildNode(*nodes[1], pages[0]->GetObject());
    appendChildNode(*nodes[1], pages[1]->GetObject());

    if (createCycle)
    {
        // invalid tree: Cycle!!!
        // was not detected in PdfPagesTree::GetPageNode() rev. 1937
        nodes[0]->GetDictionary().MustFindKey("Kids").GetArray()[0] = root.GetIndirectReference();
    }
}

void createEmptyKidsTree(PdfMemDocument& doc)
{
    const unsigned COUNT = 3;

    vector<PdfPage*> pages = createSamplePages(doc, COUNT);
    vector<PdfObject*> nodes = createNodes(doc, 3);

    // manually insert pages into pagetree
    PdfObject& root = doc.GetPageTree().GetObject();

    // tree layout:
    //
    //    root
    //    +-- node0
    //    |   +-- page0
    //    |   +-- page1
    //    |   +-- page2
    //    +-- node1
    //    \-- node2

    // root node
    appendChildNode(root, *nodes[0]);
    appendChildNode(root, *nodes[1]);
    appendChildNode(root, *nodes[2]);

    // tree node 0
    appendChildNode(*nodes[0], pages[0]->GetObject());
    appendChildNode(*nodes[0], pages[1]->GetObject());
    appendChildNode(*nodes[0], pages[2]->GetObject());

    // tree node 1 and node 2 are left empty: this is completely valid
    // according to the PDF spec, i.e. the required keys may have the
    // values "/Kids [ ]" and "/Count 0"
}

void createNestedArrayTree(PdfMemDocument& doc)
{
    constexpr unsigned COUNT = 3;

    vector<PdfPage*> pages = createSamplePages(doc, COUNT);
    PdfObject& root = doc.GetPageTree().GetObject();

    // create kids array
    PdfArray kids;
    for (unsigned i = 0; i < COUNT; i++)
    {
        kids.Add(pages[i]->GetObject().GetIndirectReference());
        pages[i]->GetObject().GetDictionary().AddKey("Parent", root.GetIndirectReference());
    }

    // create nested kids array
    PdfArray nested;
    nested.Add(kids);

    // manually insert pages into pagetree
    root.GetDictionary().AddKey("Count", static_cast<int64_t>(COUNT));
    root.GetDictionary().AddKey("Kids", nested);
}

bool isPageNumber(PdfPage& page, unsigned number)
{
    int64_t pageNumber = page.GetObject().GetDictionary().GetKeyAs<int64_t>(PODOFO_TEST_PAGE_KEY, -1);

    if (pageNumber != static_cast<int64_t>(number))
    {
        INFO(cmn::Format("PagesTreeTest: Expected page number {} but got {}", number, pageNumber));
        return false;
    }
    else
        return true;
}

void appendChildNode(PdfObject& parent, PdfObject& child)
{
    // 1. Add the reference of the new child to the kids array of parent
    PdfArray kids;
    PdfObject* oldKids = parent.GetDictionary().FindKey("Kids");
    if (oldKids != nullptr && oldKids->IsArray()) kids = oldKids->GetArray();
    kids.Add(child.GetIndirectReference());
    parent.GetDictionary().AddKey("Kids", kids);

    // 2. If the child is a page (leaf node), increase count of every parent
    //    (which also includes pParent)
    if (child.GetDictionary().GetKeyAs<PdfName>("Type") == "Page")
    {
        PdfObject* node = &parent;
        while (node)
        {
            int64_t count = 0;
            if (node->GetDictionary().FindKey("Count")) count = node->GetDictionary().FindKey("Count")->GetNumber();
            count++;
            node->GetDictionary().AddKey("Count", count);
            node = node->GetDictionary().FindKey("Parent");
        }
    }

    // 3. Add Parent key to the child
    child.GetDictionary().AddKey("Parent", parent.GetIndirectReference());
}
