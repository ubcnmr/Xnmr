main(){

char cdata[8];
unsigned int *dummy,*dum2,*dum3;

int i,j;

 dum2 = (unsigned int *) &cdata[0];
 dum3 = (unsigned int *) &cdata[4];

 for (i=0;i<4;i++){
   dummy = (unsigned int *) &cdata[i];
   *dum2 = 0;
   *dum3=0;
   *dummy = 1;
   printf("byte %i pos 1: %u %u\n",i, *dum2,*dum3);

   *dummy = 4;
   printf("byte %i pos 4: %u %u\n",i, *dum2,*dum3);

   *dummy = 32768;
   printf("byte %i pos 32: %u %u\n",i, *dum2,*dum3);

 }

}
