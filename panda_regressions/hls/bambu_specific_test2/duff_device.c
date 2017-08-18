void duff(int count, int * to, int * from)
{
 switch (count % 8)  /* count > 0 assumed */
 {
   case 0:        do {  *to = *from++;
   case 7:              *to = *from++;
   case 6:              *to = *from++;
   case 5:              *to = *from++;
   case 4:              *to = *from++;
   case 3:              *to = *from++;
   case 2:              *to = *from++;
   case 1:              *to = *from++;
                     } while ((count -= 8) > 0);
  }
}
