int delay(int ritardo)
{
	int i;	
	static int somma;
	somma=0;	
	for (i=0;i<ritardo;i++){
		somma+=i;
	}
	return somma;
}
