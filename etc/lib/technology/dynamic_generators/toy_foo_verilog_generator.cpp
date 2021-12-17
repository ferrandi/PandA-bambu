std::string expr = "";

std::string op = "-";

if((_np - 1) % 2 == 0)
   op = "+";

for(int i = 1; i < _np; i++)
{
   if(i == 1)
      expr = _p[i].name;
   else
      expr += op + _p[i].name;
}

std::cout << "assign out1=" << expr << ";\n";
