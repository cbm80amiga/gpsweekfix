// GPS week number rollover fix for Wintec WBT100/WBT200 gpslogger .tks data file
// (c) 2019 Pawel A. Hernik
//
// USAGE: gpsroll from_file to_file [offset]
//        offset=1 for winter time, offset=0 for summer time (for GoogleEarth format)

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>

int main(int argc, char **argv) 
{
  int offset=0;
  if(argc<2) {
    printf("Usage: gpsweekfix from_file to_file [offset]\n");
    return 1;
  }
  if(argc>2) offset=atoi(argv[3]);
  char *name1 = argv[1];
  char *name2 = argv[2];
  printf("params: %s %s %d\n",name1,name2,offset);
  FILE *fr = fopen(name1,"rb");
  if(!fr) {
    printf("Can't open input file [%s] !\n",name1);
    return 1;
  }
  FILE *fw = fopen(name2,"wb");
  if(!fw) {
    printf("Can't open output file [%s] !\n",name2);
    return 1;
  }
  char buf[20];

  while(!feof(fr)) {
    size_t rc = fread(buf, 16, 1, fr);
    if(rc==0) {
      printf("No more input data\n");
      break;
    }
    long tim = *((long*)buf);
    float lat = *((long*)buf+1)/10000000.0;
    float lon = *((long*)buf+2)/10000000.0;
    float alt = *((long*)buf+3)/10.0;
    int newp = 0;
    if(lat >= 100) {
       lat -= 100; newp = 1; 
    } else if(lat <= -100) {
       lat += 100; newp = 1; 
    }
    time_t nsec;
    struct tm t1, t2, *tt;
    t1.tm_sec  = (tim >>  0) & 0x3F;
    t1.tm_min  = (tim >>  6) & 0x3F; 
    t1.tm_hour = (tim >> 12) & 0x1F; 
    t1.tm_mday = (tim >> 17) & 0x1F; 
    t1.tm_mon  = ((tim >> 22) & 0x0F)-1; 
    t1.tm_year = ((tim >> 26) & 0x3F)+100;
    //printf("orig %02d-%02d-%02d %02d:%02d:%02d %.6f %.6f %.2f\n",t1.tm_year,t1.tm_mon,t1.tm_mday,t1.tm_hour,t1.tm_min,t1.tm_sec,lat,lon,alt);
    nsec = mktime(&t1);
    nsec += 1024*7*24*60*60;  // gps week rollover patch (+1024 weeks)
    nsec += 60*60*offset;     // time zone offset
    tt = localtime(&nsec);
    if(!tt) {
       printf("Can't convert time for orig year=%d nsec=%lx too big!\n",1900+t1.tm_year,nsec); 
       continue; 
    }
    memcpy(&t2,tt,sizeof(struct tm));  // safe copy
    printf("%02d-%02d-%02d %02d:%02d:%02d %.6f %.6f %.2f %s\n",1900-1+t2.tm_year,t2.tm_mon+1,t2.tm_mday,t2.tm_hour,t2.tm_min,t2.tm_sec,lat,lon,alt,newp?" --- new path ---":"");
    long tim2 = (((t2.tm_year-100-1)&0x3f)<<26) | (((t2.tm_mon+1)&0xf)<<22) | ((t2.tm_mday&0x1f)<<17) |
                 ((t2.tm_hour&0x1f)<<12) | ((t2.tm_min&0x3f)<<6) | ((t2.tm_sec&0x3f)<<0);
    //t2.tm_year-100-1 -> 2000->2019
   *((long*)buf)=tim2;
    rc = fwrite(buf, 16, 1, fw);
  }
  fclose(fr);
  fclose(fw);
  return 0;
}
