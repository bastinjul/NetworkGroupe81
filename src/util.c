#include <sys/time.h>
#include <stdint.h>

uint32_t timeval_to_millisec(struct timeval* timev){
  return (uint32_t)(timev->tv_sec * 1000) + (uint32_t)(timev->tv_usec / 1000);
}
