#include <stdio.h>
int main (int argc, char **argv){
  printf("You are driving straight.\n");
  // printf("Press 's' to go straight, 'l' to turn left, or 'r' to turn right.\n");

  char state = 's';
  char c[2];
  while(1){
    if(state == 's') {
      printf("Press 's' to go straight, 'l' to turn left, or 'r' to turn right.\n");
    } else {
      printf("Press 's' to go straight\n");
    }

    gets(c);

    if(state == 's') {
      state = c[0];
      if(c[0] == 'l') printf("Turning left\n");
      else if (c[0] == 'r') printf("Turning right\n");
      else if (c[0] == 's') printf("Going straight\n");
    } else{
      if(c[0]== 's') {
        printf("Going straight\n");
        state = c[0];
      }
      else {
        printf("Illegal input. You are crashing. Try again.\n");
      }
    }
  }
  return 0;
}
