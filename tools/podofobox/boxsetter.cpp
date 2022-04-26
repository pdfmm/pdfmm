/***************************************************************************
 *   Copyright (C) 2010 by Pierre Marchand   *
 *   pierre@oep-h.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "boxsetter.h"

using namespace std;
using namespace PoDoFo;

BoxSetter::BoxSetter(const string_view& in, const string& out, const string_view& box, const PdfRect& rect)
    : m_box(box), m_rect(rect)
{
    PdfMemDocument source;
    source.Load(in);
    int pcount(source.GetPages().GetCount());
    for (int i = 0; i < pcount; ++i)
        SetBox(source.GetPages().GetPage(i));

    source.Save(out);
}

void BoxSetter::SetBox(PdfPage& page)
{
    PdfArray r;
    m_rect.ToArray(r);
    if (m_box.find("media") != string::npos)
        page.GetObject().GetDictionary().AddKey("MediaBox", r);
    else if (m_box.find("crop") != string::npos)
        page.GetObject().GetDictionary().AddKey("CropBox", r);
    else if (m_box.find("bleed") != string::npos)
        page.GetObject().GetDictionary().AddKey("BleedBox", r);
    else if (m_box.find("trim") != string::npos)
        page.GetObject().GetDictionary().AddKey("TrimBox", r);
    else if (m_box.find("art") != string::npos)
        page.GetObject().GetDictionary().AddKey("ArtBox", r);

    // TODO check that box sizes are ordered
}

bool BoxSetter::CompareBox(const PdfRect &rect1, const PdfRect &rect2)
{
	return rect1.ToString() == rect2.ToString();
}
