void delay(int ritardo)
{
	int i;	
	volatile int somma;
	somma=0;	
	for (i=0;i<ritardo;i++)
		somma+=i;
}
