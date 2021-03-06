/*
  Alazar Shenkute
  05/05/2016
  Command line expects 4 arguments:
  (1) the length of the time the program should run, 
  (2) the number of producer threads
  (3) the number of middleman threads
  (4) the number of consumer threads
*/
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<semaphore.h>

#define BUFFER_SIZE 5
#define PRODUCER_MODE 1
#define MIDDLEMAN_MODE 2
#define CONSUMER_MODE 3
typedef int buffer_item;


/* the buffers 
   producer buffer and consumer buffer */
buffer_item producer_buffer[BUFFER_SIZE];
buffer_item consumer_buffer[BUFFER_SIZE];
/* shared data between threads */
pthread_mutex_t producer_lock;
pthread_mutex_t consumer_lock;
int producer_in;
int producer_out;
int consumer_in;
int consumer_out;
//  
sem_t empty;
sem_t full;

void printBuffer()
{
  int i = 0;
  for( i = 0 ; i < BUFFER_SIZE; i++ )
    {
      printf( "%s%d%s", "[", producer_buffer[i] ,"]");
    }
  printf( "%s%d%s%d\n"," in = ", producer_in , " out = ", producer_out );
  for( i = 0 ; i < BUFFER_SIZE; i++ )
    {
      printf( "%s%d%s", "[", consumer_buffer[i] ,"]");
    }
  printf( "%s%d%s%d\n"," in = ", consumer_in , " out = ", consumer_out );
}

/* insert item into buffer
  return 0 if success, otherwise 
  return -1
  argument mode determines whether this function was 
  invoked by middleman thread or by producer thread
*/
int insert_item( buffer_item item, int mode )
{
  if( mode == PRODUCER_MODE )
    { 
      sem_wait( &empty );
      pthread_mutex_lock( &producer_lock );
      producer_buffer[producer_in] = item;
      printf( "%s%d%s%d\n", "insert_item inserted item " , item , " at position ", producer_in);
      producer_in = ( producer_in + 1 ) % BUFFER_SIZE;
      printBuffer();
      pthread_mutex_unlock( &producer_lock );
      sem_post( &full );
    }else{

    sem_wait( &empty );
    pthread_mutex_lock( &consumer_lock );
    consumer_buffer[producer_in] = item;
    printf( "%s%d%s%d\n", "insert_item inserted item " , item , " at position ", consumer_in);
    consumer_in = ( consumer_in + 1 ) % BUFFER_SIZE;
    printBuffer();
    pthread_mutex_unlock( &consumer_lock );
    sem_post( &full );

  }
  return 0;

}
/* remove item into buffer
  return 0 if success, otherwise 
  return -1 
*/
int remove_item( buffer_item *item, int mode )
{
  sem_wait( &full );
  pthread_mutex_lock( &producer_lock );
  *item = producer_buffer[producer_out];
  printf( "%s%d%s%d\n", "remove_item removed item ", *item, " at position ", producer_out );
  producer_buffer[producer_out] = 0;
  producer_out = ( producer_out + 1 )% BUFFER_SIZE;
  printBuffer();
  pthread_mutex_unlock( &producer_lock );
  sem_post( &empty );
  return 0;
}

/*
  middle man takes items from the producer buffer
  and puts it in consumer buffer
*/
void *middleman( void *param )
{
  printf("%s\n", "INSIDE MIDDLEMAN THREAD");
  buffer_item item;
  while( 1 )
    {
      int sleepTime = ( rand() % 3 ) + 1;
      printf("%s%lu%s%d\n", "middleman thread ", pthread_self() ,  " sleeping for ", sleepTime );
      sleep( sleepTime );
    
      if( remove_item( &item, PRODUCER_MODE ) < 0 ){
        printf( "Middleman error when removing item\n");
      }
      printf( "%s%lu%s%d\n", "Middleman thread ", pthread_self(), " removed ", item );     

      if( insert_item( item, CONSUMER_MODE ) ){
        printf( "Middleman error when inserting item\n");
      }  
      printf( "%s%lu%s%d\n", "Middleman thread ", pthread_self(), " inserted ", item );
    }

}
void *producer( void *param )
{
  buffer_item item;
  // runs forever
  while( 1 )
    {
      /* generate random number */
      /* sleep for random amount of time */
      int sleepTime = ( rand() % 3 ) + 1;
      printf("%s%lu%s%d\n", "producer thread ", pthread_self() ,  " sleeping for ", sleepTime );
      sleep( sleepTime );
      item = ( rand() % 100 ) + 1;
      printf( "producer produced %d\n", item );

      if( insert_item( item, PRODUCER_MODE ) < 0 ){
        printf( "Producer error\n");
      }
      printf( "%s%lu%s%d\n", "Producer thread ", pthread_self(), " inserted ", item );
    }
}

void *consumer( void *param )
{
  buffer_item item;
  // runs forever
  while( 1 )
    {
      /* generate random number 
	 sleep for ramom amount of time */
      int sleepTime = ( rand() % 3 ) + 1;
      printf("%s%lu%s%d\n", "Consumer thread ", pthread_self() ,  " sleeping for ", sleepTime );
      sleep( sleepTime );
      if( remove_item( &item, CONSUMER_MODE ) < 0 ){
	printf( "consumer error\n" );
      }else{
	printf( "%s%lu%s%d\n", "Consumer thread ", pthread_self(), " removed ", item );          
      }

    }
}
/* initialize buffer, mutual exclusion object,
   and empty and full semaphores
*/
void init(){
  int i;
  for( i = 0; i < BUFFER_SIZE; i++ ){
    producer_buffer[i] = 0;
    consumer_buffer[i] = 0;
  }
  // empty = BUFFER_SIZE;
  producer_in = producer_out = consumer_in = consumer_out = 0;
  printf("%d\n", empty );
  sem_init( &empty, 0, 5 );
  sem_init( &full, 0, 0 );
  printf("empty after initiliaztion = %d\n", empty );
}

int main( int argc, char*argv[] )
{
  if( argc < 3 ){
    printf( "Invalid # of arguments" );
    return -1;
  }
  printf( "Main thread begining\n" );
  /* Get command line arguments */
  int programTime = atoi( argv[1] );
  int numOfConsumerThreads = atoi( argv[2] );
  int numOfMiddlemanThreads = atoi( argv[3]);
  int numOfProducerThreads = atoi( argv[4] );
  int nThreads = 0;
  // 
  pthread_t thr[numOfConsumerThreads+numOfProducerThreads];

  // initialze mutex object and semaphores
  init();            
  // create producer threads
  int i, rc;
  for( i = 0; i < numOfProducerThreads; i++ ){
    if( (rc = pthread_create( &thr[i], NULL, producer, NULL )) ) {
      fprintf(stderr, "error: pthread_create, rc: %d\n", rc );
      return EXIT_FAILURE;
    }
  }
  printf( "Creating producer thread with id %d\n", pthread_self() );
  
  // create middleman threads
  for( i = 0; i < numOfMiddlemanThreads; i++ ){
    if( (rc = pthread_create( &thr[i], NULL, middleman, NULL )) ) {
      fprintf(stderr, "error: pthread_create, rc: %d\n", rc );
      return EXIT_FAILURE;
    }
  }
  printf( "Creating middleman thread with id %d\n", pthread_self() );
    
  // create consumer threads
  for( i = 0; i < numOfConsumerThreads; i++ ){
    if( (rc = pthread_create( &thr[i], NULL, consumer, NULL )) ) {
      fprintf(stderr, "error: pthread_create, rc: %d\n", rc );
      return EXIT_FAILURE;
    }
  }
  printf( "Creating consumer thread with id %d\n", pthread_self() );
  /* Sleep */
  printf("Main thread sleeping for %d\n", programTime );
  sleep( programTime );
  printf( "Main thread exiting\nBye..\n" );

}

