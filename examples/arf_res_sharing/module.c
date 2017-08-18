/*

     ARF: digital implementation of an  auto-regressive lattice filter. The benchmark consists of 16 multiplication and 12 addition operations. The ARF benchmark operates in a loop. This feature is exploited by exercising the simulation as closed loop system.
   
     from:
     N. Mukherjee:
     "Built-in in Self-Test for Functional Blocks in Data-Path Architectures", PhD thesis, McGill University, Montreal, 1996.

     where it is referred to:
     R. Jain:
    "High-Level Area-Delay Prediction with Application to Behavioral Synthesis", PhD thesis, University of Southern California, Los Angeles, Usa, 1989.
     

*/

void arf(int i1,int i2,int i3,int i4,int i5,int i6,
         int *o1,int *o2,int *o3,int *o4,int G1,int G2, int G3, int G4, int GG1, int GG2){

  int op1,op2,op3,op4,op5,op6,op7,op8; 
  int op9,op10,op11,op12,op13,op14,op15,op16;
  int op17,op18,op19,op20,op21,op22,op23,op24;
  int op25,op26,op27,op28;
 
  op1 = GG1 * i1;
  op2 = GG2 * i2;
  op3 = G1 * i2;
  op4 = G2 * i1;
  op5 = G1 * i3;
  op6 = G2 * i4;
  op7 = G1 * i4;
  op8 = G2 * i3;
  
  op9 = op1 + op2;
  op10 = op3 + op4;
  op11 = op5 + op6;
  op12 = op7 + op8;

  op13 = op11 + i5;
  *o1 = op13;
  op14 = i6 + op12;
  *o2 = op14;
  op15 = G1 * op14;
  op16 = op13 * G2;
  op17 = op13 * G1;
  op18 = op14 * G2;
  op19 = op15 * op16;
  op20 = op17 + op18;
  op21 = G1 * op20;
  op22 = op19 * G2;
  op23 = op19 * G1; 
  op24 = op20 * G2;
  op25 = op21 + op22;
  op26 = op23 + op24;
  op27 = op9 + op25;
  *o3 = op27;
  op28 = op10 + op26;
  *o4 = op28;

}
