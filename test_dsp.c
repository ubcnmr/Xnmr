
int setup_dsp(int sw, double freq);
main(){

int i,j;
double fr;
float data[8192*2];

 fr=4100000.;
 i=setup_dsp(1000000,fr);

 sleep(1);
 j=read_fifo(8192,data);
 for(i=0;i<8192;i++)
   printf("%0.f %0.f\n",data[2*i],data[2*i+1]);

 i=setup_dsp(1000000,fr);
 i=setup_dsp(20000,fr);

 sleep(1);
 j=read_fifo(8192,data);
 for(i=0;i<8192;i++)
   printf("%0.f %0.f\n",data[2*i],data[2*i+1]);


}
