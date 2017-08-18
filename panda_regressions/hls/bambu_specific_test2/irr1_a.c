extern void irr3_empty();
void duff(int count, int * to, int * from)
{
 irr3_empty();
 switch (count % 8)  /* count > 0 assumed */
 {
   case 0:        do {  *to = *from++; irr3_empty();
   case 7:              *to = *from++; irr3_empty();
   case 6:              *to = *from++; irr3_empty();
   case 5:              *to = *from++; irr3_empty();
   case 4:              *to = *from++; irr3_empty();
   case 3:              *to = *from++; irr3_empty();
   case 2:              *to = *from++; irr3_empty();
   case 1:              *to = *from++; irr3_empty();
                     } while ((count -= 8) > 0);
  }
  irr3_empty();
}
