int memory(int base, int i)
{
   static char str[] = {'a','b','c','d'}; 
   //,'e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'};
   int flag = 0;
   flag += str[i] - base;
   i++;
   /**
    * Adding any other computation results in random values assigned to flag,
    * the same behavior can be observed simulating without flag -p
    */
   //flag += str[i] - base - i;
   //i++;
   //flag += str[i] - base - i;
   //i++;
   //flag += str[i] - base - i;
   //i++;
   return flag;
}
