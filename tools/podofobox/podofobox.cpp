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

#include <podofo/podofo.h>

#include <string>
#include <iostream>

#include "boxsetter.h"

using namespace std;
using namespace PoDoFo;

void print_help()
{
    cerr << "Usage: podofobox [inputfile] [outpufile] [box] [left] [bottom] [width] [height]" << endl;
    cerr << "Box is one of media crop bleed trim art." << endl;
    cerr << "Give values * 100 as integers (avoid locale headaches with strtod)" << endl;
    cerr << endl << endl << "PoDoFo Version: " << PODOFO_VERSION_STRING << endl << endl;
}

int main(int argc, char* argv[])
{
    if (argc != 8)
    {
        print_help();
        exit(-1);
    }

    string input = argv[1];
    string output = argv[2];
    string box = argv[3];

    double left = double(atol(argv[4])) / 100.0;
    double bottom = double(atol(argv[5])) / 100.0;
    double width = double(atol(argv[6])) / 100.0;
    double height = double(atol(argv[7])) / 100.0;
    PdfRect rect(left, bottom, width, height);

    try
    {
        BoxSetter bs(input, output, box, rect);
    }
    catch (PdfError& e)
    {
        cerr << "Error: An error " << e.what() << " ocurred during processing the pdf file" << endl;
        e.PrintErrorMsg();
        return (int)e.GetError();
    }

    return 0;
}
