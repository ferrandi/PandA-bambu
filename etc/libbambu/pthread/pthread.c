typedef int pthread_mutex_t;
extern void panda_pthread_mutex(unsigned int mutex, unsigned int locking);

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
   panda_pthread_mutex((unsigned int) mutex, 1);
   return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
   panda_pthread_mutex((unsigned int) mutex, 0);
   return 0;
}
