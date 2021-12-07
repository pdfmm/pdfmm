#include "OperatorUtils.h"

using namespace std;
using namespace mm;

bool mm::TryGetPDFOperator(const string_view& opstr, PdfContentOperator& op)
{
    if (opstr == "w")
    {
        op = PdfContentOperator::w;
        return true;
    }
    else if (opstr == "J")
    {
        op = PdfContentOperator::J;
        return true;
    }
    else if (opstr == "j")
    {
        op = PdfContentOperator::j;
        return true;
    }
    else if (opstr == "M")
    {
        op = PdfContentOperator::M;
        return true;
    }
    else if (opstr == "d")
    {
        op = PdfContentOperator::d;
        return true;
    }
    else if (opstr == "ri")
    {
        op = PdfContentOperator::ri;
        return true;
    }
    else if (opstr == "i")
    {
        op = PdfContentOperator::i;
        return true;
    }
    else if (opstr == "gs")
    {
        op = PdfContentOperator::gs;
        return true;
    }
    else if (opstr == "q")
    {
        op = PdfContentOperator::q;
        return true;
    }
    else if (opstr == "Q")
    {
        op = PdfContentOperator::Q;
        return true;
    }
    else if (opstr == "cm")
    {
        op = PdfContentOperator::cm;
        return true;
    }
    else if (opstr == "m")
    {
        op = PdfContentOperator::m;
        return true;
    }
    else if (opstr == "l")
    {
        op = PdfContentOperator::l;
        return true;
    }
    else if (opstr == "c")
    {
        op = PdfContentOperator::c;
        return true;
    }
    else if (opstr == "v")
    {
        op = PdfContentOperator::v;
        return true;
    }
    else if (opstr == "y")
    {
        op = PdfContentOperator::y;
        return true;
    }
    else if (opstr == "h")
    {
        op = PdfContentOperator::h;
        return true;
    }
    else if (opstr == "re")
    {
        op = PdfContentOperator::re;
        return true;
    }
    else if (opstr == "S")
    {
        op = PdfContentOperator::S;
        return true;
    }
    else if (opstr == "s")
    {
        op = PdfContentOperator::s;
        return true;
    }
    else if (opstr == "f")
    {
        op = PdfContentOperator::f;
        return true;
    }
    else if (opstr == "F")
    {
        op = PdfContentOperator::F;
        return true;
    }
    else if (opstr == "f*")
    {
        op = PdfContentOperator::f_Star;
        return true;
    }
    else if (opstr == "B")
    {
        op = PdfContentOperator::B;
        return true;
    }
    else if (opstr == "B*")
    {
        op = PdfContentOperator::B_Star;
        return true;
    }
    else if (opstr == "b")
    {
        op = PdfContentOperator::b;
        return true;
    }
    else if (opstr == "b*")
    {
        op = PdfContentOperator::b_Star;
        return true;
    }
    else if (opstr == "n")
    {
        op = PdfContentOperator::n;
        return true;
    }
    else if (opstr == "W")
    {
        op = PdfContentOperator::W;
        return true;
    }
    else if (opstr == "W*")
    {
        op = PdfContentOperator::W_Star;
        return true;
    }
    else if (opstr == "BT")
    {
        op = PdfContentOperator::BT;
        return true;
    }
    else if (opstr == "ET")
    {
        op = PdfContentOperator::ET;
        return true;
    }
    else if (opstr == "Tc")
    {
        op = PdfContentOperator::Tc;
        return true;
    }
    else if (opstr == "Tw")
    {
        op = PdfContentOperator::Tw;
        return true;
    }
    else if (opstr == "Tz")
    {
        op = PdfContentOperator::Tz;
        return true;
    }
    else if (opstr == "TL")
    {
        op = PdfContentOperator::TL;
        return true;
    }
    else if (opstr == "Tf")
    {
        op = PdfContentOperator::Tf;
        return true;
    }
    else if (opstr == "Tr")
    {
        op = PdfContentOperator::Tr;
        return true;
    }
    else if (opstr == "Ts")
    {
        op = PdfContentOperator::Ts;
        return true;
    }
    else if (opstr == "Td")
    {
        op = PdfContentOperator::Td;
        return true;
    }
    else if (opstr == "TD")
    {
        op = PdfContentOperator::TD;
        return true;
    }
    else if (opstr == "Tm")
    {
        op = PdfContentOperator::Tm;
        return true;
    }
    else if (opstr == "T*")
    {
        op = PdfContentOperator::T_Star;
        return true;
    }
    else if (opstr == "Tj")
    {
        op = PdfContentOperator::Tj;
        return true;
    }
    else if (opstr == "TJ")
    {
        op = PdfContentOperator::TJ;
        return true;
    }
    else if (opstr == "'")
    {
        op = PdfContentOperator::Quote;
        return true;
    }
    else if (opstr == "\"")
    {
        op = PdfContentOperator::DoubleQuote;
        return true;
    }
    else if (opstr == "d0")
    {
        op = PdfContentOperator::d0;
        return true;
    }
    else if (opstr == "d1")
    {
        op = PdfContentOperator::d1;
        return true;
    }
    else if (opstr == "CS")
    {
        op = PdfContentOperator::CS;
        return true;
    }
    else if (opstr == "cs")
    {
        op = PdfContentOperator::cs;
        return true;
    }
    else if (opstr == "SC")
    {
        op = PdfContentOperator::SC;
        return true;
    }
    else if (opstr == "SCN")
    {
        op = PdfContentOperator::SCN;
        return true;
    }
    else if (opstr == "sc")
    {
        op = PdfContentOperator::sc;
        return true;
    }
    else if (opstr == "scn")
    {
        op = PdfContentOperator::scn;
        return true;
    }
    else if (opstr == "G")
    {
        op = PdfContentOperator::G;
        return true;
    }
    else if (opstr == "g")
    {
        op = PdfContentOperator::g;
        return true;
    }
    else if (opstr == "RG")
    {
        op = PdfContentOperator::RG;
        return true;
    }
    else if (opstr == "rg")
    {
        op = PdfContentOperator::rg;
        return true;
    }
    else if (opstr == "K")
    {
        op = PdfContentOperator::K;
        return true;
    }
    else if (opstr == "k")
    {
        op = PdfContentOperator::k;
        return true;
    }
    else if (opstr == "sh")
    {
        op = PdfContentOperator::sh;
        return true;
    }
    else if (opstr == "BI")
    {
        op = PdfContentOperator::BI;
        return true;
    }
    else if (opstr == "ID")
    {
        op = PdfContentOperator::ID;
        return true;
    }
    else if (opstr == "EI")
    {
        op = PdfContentOperator::EI;
        return true;
    }
    else if (opstr == "Do")
    {
        op = PdfContentOperator::Do;
        return true;
    }
    else if (opstr == "MP")
    {
        op = PdfContentOperator::MP;
        return true;
    }
    else if (opstr == "DP")
    {
        op = PdfContentOperator::DP;
        return true;
    }
    else if (opstr == "BMC")
    {
        op = PdfContentOperator::BMC;
        return true;
    }
    else if (opstr == "BDC")
    {
        op = PdfContentOperator::BDC;
        return true;
    }
    else if (opstr == "EMC")
    {
        op = PdfContentOperator::EMC;
        return true;
    }
    else if (opstr == "BX")
    {
        op = PdfContentOperator::BX;
        return true;
    }
    else if (opstr == "EX")
    {
        op = PdfContentOperator::EX;
        return true;
    }
    else
    {
        op = PdfContentOperator::Unknown;
        return false;
    }
}

bool mm::TryGetString(PdfContentOperator op, string_view& opstr)
{
    switch (op)
    {
        case PdfContentOperator::w:
        {
            opstr = "w";
            return true;
        }
        case PdfContentOperator::J:
        {
            opstr = "J";
            return true;
        }
        case PdfContentOperator::j:
        {
            opstr = "j";
            return true;
        }
        case PdfContentOperator::M:
        {
            opstr = "M";
            return true;
        }
        case PdfContentOperator::d:
        {
            opstr = "d";
            return true;
        }
        case PdfContentOperator::ri:
        {
            opstr = "ri";
            return true;
        }
        case PdfContentOperator::i:
        {
            opstr = "i";
            return true;
        }
        case PdfContentOperator::gs:
        {
            opstr = "gs";
            return true;
        }
        case PdfContentOperator::q:
        {
            opstr = "q";
            return true;
        }
        case PdfContentOperator::Q:
        {
            opstr = "Q";
            return true;
        }
        case PdfContentOperator::cm:
        {
            opstr = "cm";
            return true;
        }
        case PdfContentOperator::m:
        {
            opstr = "m";
            return true;
        }
        case PdfContentOperator::l:
        {
            opstr = "l";
            return true;
        }
        case PdfContentOperator::c:
        {
            opstr = "c";
            return true;
        }
        case PdfContentOperator::v:
        {
            opstr = "v";
            return true;
        }
        case PdfContentOperator::y:
        {
            opstr = "y";
            return true;
        }
        case PdfContentOperator::h:
        {
            opstr = "h";
            return true;
        }
        case PdfContentOperator::re:
        {
            opstr = "re";
            return true;
        }
        case PdfContentOperator::S:
        {
            opstr = "S";
            return true;
        }
        case PdfContentOperator::s:
        {
            opstr = "s";
            return true;
        }
        case PdfContentOperator::f:
        {
            opstr = "f";
            return true;
        }
        case PdfContentOperator::F:
        {
            opstr = "F";
            return true;
        }
        case PdfContentOperator::f_Star:
        {
            opstr = "f*";
            return true;
        }
        case PdfContentOperator::B:
        {
            opstr = "B";
            return true;
        }
        case PdfContentOperator::B_Star:
        {
            opstr = "B*";
            return true;
        }
        case PdfContentOperator::b:
        {
            opstr = "b";
            return true;
        }
        case PdfContentOperator::b_Star:
        {
            opstr = "b*";
            return true;
        }
        case PdfContentOperator::n:
        {
            opstr = "n";
            return true;
        }
        case PdfContentOperator::W:
        {
            opstr = "W";
            return true;
        }
        case PdfContentOperator::W_Star:
        {
            opstr = "W*";
            return true;
        }
        case PdfContentOperator::BT:
        {
            opstr = "BT";
            return true;
        }
        case PdfContentOperator::ET:
        {
            opstr = "ET";
            return true;
        }
        case PdfContentOperator::Tc:
        {
            opstr = "Tc";
            return true;
        }
        case PdfContentOperator::Tw:
        {
            opstr = "Tw";
            return true;
        }
        case PdfContentOperator::Tz:
        {
            opstr = "Tz";
            return true;
        }
        case PdfContentOperator::TL:
        {
            opstr = "TL";
            return true;
        }
        case PdfContentOperator::Tf:
        {
            opstr = "Tf";
            return true;
        }
        case PdfContentOperator::Tr:
        {
            opstr = "Tr";
            return true;
        }
        case PdfContentOperator::Ts:
        {
            opstr = "Ts";
            return true;
        }
        case PdfContentOperator::Td:
        {
            opstr = "Td";
            return true;
        }
        case PdfContentOperator::TD:
        {
            opstr = "TD";
            return true;
        }
        case PdfContentOperator::Tm:
        {
            opstr = "Tm";
            return true;
        }
        case PdfContentOperator::T_Star:
        {
            opstr = "T*";
            return true;
        }
        case PdfContentOperator::Tj:
        {
            opstr = "Tj";
            return true;
        }
        case PdfContentOperator::TJ:
        {
            opstr = "TJ";
            return true;
        }
        case PdfContentOperator::Quote:
        {
            opstr = "'";
            return true;
        }
        case PdfContentOperator::DoubleQuote:
        {
            opstr = "\"";
            return true;
        }
        case PdfContentOperator::d0:
        {
            opstr = "d0";
            return true;
        }
        case PdfContentOperator::d1:
        {
            opstr = "d1";
            return true;
        }
        case PdfContentOperator::CS:
        {
            opstr = "CS";
            return true;
        }
        case PdfContentOperator::cs:
        {
            opstr = "cs";
            return true;
        }
        case PdfContentOperator::SC:
        {
            opstr = "SC";
            return true;
        }
        case PdfContentOperator::SCN:
        {
            opstr = "SCN";
            return true;
        }
        case PdfContentOperator::sc:
        {
            opstr = "sc";
            return true;
        }
        case PdfContentOperator::scn:
        {
            opstr = "scn";
            return true;
        }
        case PdfContentOperator::G:
        {
            opstr = "G";
            return true;
        }
        case PdfContentOperator::g:
        {
            opstr = "g";
            return true;
        }
        case PdfContentOperator::RG:
        {
            opstr = "RG";
            return true;
        }
        case PdfContentOperator::rg:
        {
            opstr = "rg";
            return true;
        }
        case PdfContentOperator::K:
        {
            opstr = "K";
            return true;
        }
        case PdfContentOperator::k:
        {
            opstr = "k";
            return true;
        }
        case PdfContentOperator::sh:
        {
            opstr = "sh";
            return true;
        }
        case PdfContentOperator::BI:
        {
            opstr = "BI";
            return true;
        }
        case PdfContentOperator::ID:
        {
            opstr = "ID";
            return true;
        }
        case PdfContentOperator::EI:
        {
            opstr = "EI";
            return true;
        }
        case PdfContentOperator::Do:
        {
            opstr = "Do";
            return true;
        }
        case PdfContentOperator::MP:
        {
            opstr = "MP";
            return true;
        }
        case PdfContentOperator::DP:
        {
            opstr = "DP";
            return true;
        }
        case PdfContentOperator::BMC:
        {
            opstr = "BMC";
            return true;
        }
        case PdfContentOperator::BDC:
        {
            opstr = "BDC";
            return true;
        }
        case PdfContentOperator::EMC:
        {
            opstr = "EMC";
            return true;
        }
        case PdfContentOperator::BX:
        {
            opstr = "BX";
            return true;
        }
        case PdfContentOperator::EX:
        {
            opstr = "EX";
            return true;
        }
        case PdfContentOperator::Unknown:
        default:
            opstr = { };
            return true;
    }
}
